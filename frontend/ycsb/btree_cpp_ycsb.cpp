#include <BtreeCppPerfEvent.hpp>
#include "../shared/BTreeCppAdapter.hpp"
#include "../shared/Schema.hpp"
#include "Units.hpp"
#include "leanstore/Config.hpp"
#include "leanstore/LeanStore.hpp"
#include "leanstore/profiling/counters/WorkerCounters.hpp"
#include "leanstore/utils/FVector.hpp"
#include "leanstore/utils/Files.hpp"
#include "leanstore/utils/Parallelize.hpp"
#include "leanstore/utils/RandomGenerator.hpp"
#include "leanstore/utils/ScrambledZipfGenerator.hpp"
// -------------------------------------------------------------------------------------
#include <gflags/gflags.h>
#include <tbb/parallel_for.h>
// -------------------------------------------------------------------------------------
#include <iostream>
#include <set>
// -------------------------------------------------------------------------------------
DEFINE_uint32(ycsb_read_ratio, 100, "");
DEFINE_uint64(ycsb_tuple_count, 0, "");
DEFINE_uint32(ycsb_payload_size, 100, "tuple size in bytes");
DEFINE_uint32(ycsb_warmup_rounds, 0, "");
DEFINE_uint32(ycsb_insert_threads, 0, "");
DEFINE_uint32(ycsb_threads, 0, "");
DEFINE_bool(ycsb_count_unique_lookup_keys, true, "");
DEFINE_bool(ycsb_warmup, true, "");
DEFINE_uint32(ycsb_sleepy_thread, 0, "");
DEFINE_uint32(ycsb_ops_per_tx, 1, "");
// -------------------------------------------------------------------------------------
using namespace leanstore;
// -------------------------------------------------------------------------------------
using YCSBKey = u64;
using YCSBPayload = BytesPayload<8>;
using KVTable = Relation<YCSBKey, YCSBPayload>;
// -------------------------------------------------------------------------------------
double calculateMTPS(chrono::high_resolution_clock::time_point begin, chrono::high_resolution_clock::time_point end, u64 factor)
{
   double tps = ((factor * 1.0 / (chrono::duration_cast<chrono::microseconds>(end - begin).count() / 1000000.0)));
   return (tps / 1000000.0);
}
// -------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
   gflags::SetUsageMessage("Leanstore Frontend");
   gflags::ParseCommandLineFlags(&argc, &argv, true);
   // -------------------------------------------------------------------------------------
   chrono::high_resolution_clock::time_point begin, end;
   // -------------------------------------------------------------------------------------
   // Always init with the maximum number of threads (FLAGS_worker_threads)
   BTreeCppAdapter<KVTable> table;
   // -------------------------------------------------------------------------------------
   leanstore::TX_ISOLATION_LEVEL isolation_level = leanstore::parseIsolationLevel(FLAGS_isolation_level);
   const TX_MODE tx_type = TX_MODE::OLTP;
   // -------------------------------------------------------------------------------------
   const u64 ycsb_tuple_count = (FLAGS_ycsb_tuple_count)
                                    ? FLAGS_ycsb_tuple_count
                                    : FLAGS_target_gib * 1024 * 1024 * 1024 * 1.0 / 2.0 / (sizeof(YCSBKey) + sizeof(YCSBPayload));
   BTreeCppPerfEvent perfEvent = makePerfEvent("ycsb", false, ycsb_tuple_count);

   // Insert values
   const u64 n = ycsb_tuple_count;
   // -------------------------------------------------------------------------------------

   {
      perfEvent.setParam("op", "insert");
      BTreeCppPerfEventBlock perfEventBlock(perfEvent, n);
      begin = chrono::high_resolution_clock::now();
      for (u64 i = 0; i < n; ++i) {
         YCSBPayload payload;
         utils::RandomGenerator::getRandString(reinterpret_cast<u8*>(&payload), sizeof(YCSBPayload));
         YCSBKey key = i;
         table.insert({key}, {payload});
      }
      end = chrono::high_resolution_clock::now();
   }
   // -------------------------------------------------------------------------------------
   auto zipf_random = std::make_unique<utils::ScrambledZipfGenerator>(0, ycsb_tuple_count, FLAGS_zipf_factor);
   atomic<bool> keep_running = true;

   if (FLAGS_ycsb_sleepy_thread) {
      cerr << "threads not supported" << endl;
      throw;
   }

   uint64_t txCount = 0;

   std::thread worker([&]() {
      perfEvent.setParam("op", "tx");
      BTreeCppPerfEventBlock perfEventBlock(perfEvent, n);
      jumpmuTry() while (keep_running)
      {
         {
            YCSBKey key;
            if (FLAGS_zipf_factor == 0) {
               key = utils::RandomGenerator::getRandU64(0, ycsb_tuple_count);
            } else {
               key = zipf_random->rand();
            }
            assert(key < ycsb_tuple_count);
            YCSBPayload result;
            for (u64 op_i = 0; op_i < FLAGS_ycsb_ops_per_tx; op_i++) {
               if (FLAGS_ycsb_read_ratio == 100 || utils::RandomGenerator::getRandU64(0, 100) < FLAGS_ycsb_read_ratio) {
                  table.lookup1({key}, [&](const KVTable&) {});         // result = record.my_payload;
                  leanstore::storage::BMC::global_bf->evictLastPage();  // to ignore the replacement strategy effect on MVCC experiment
               } else {
                  UpdateDescriptorGenerator1(tabular_update_descriptor, KVTable, my_payload);
                  utils::RandomGenerator::getRandString(reinterpret_cast<u8*>(&result), sizeof(YCSBPayload));
                  // -------------------------------------------------------------------------------------
                  table.update1(
                      {key}, [&](KVTable& rec) { rec.my_payload = result; }, tabular_update_descriptor);
                  leanstore::storage::BMC::global_bf->evictLastPage();  // to ignore the replacement strategy effect on MVCC experiment
               }
            }
            ++txCount;
         }
      }
      perfEventBlock.scale = txCount;
      jumpmuCatch()
      {
         // no idea what triggers jumpmuCatch, but it hopefully does not happen if you don't use leanstore
         abort();
         WorkerCounters::myCounters().tx_abort++;
      }
   });

   // -------------------------------------------------------------------------------------
   {
      // Shutdown threads
      sleep(FLAGS_run_for_seconds);
      keep_running = false;
      worker.join();
   }
   return 0;
}
