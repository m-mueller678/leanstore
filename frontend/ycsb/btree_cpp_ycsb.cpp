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
   // Insert values
   const u64 n = ycsb_tuple_count;
   // -------------------------------------------------------------------------------------
   if (FLAGS_tmp4) {
      // -------------------------------------------------------------------------------------
      std::ofstream csv;
      csv.open("zipf.csv", ios::trunc);
      csv.seekp(0, ios::end);
      csv << std::setprecision(2) << std::fixed;
      std::unordered_map<u64, u64> ht;
      auto zipf_random = std::make_unique<utils::ScrambledZipfGenerator>(0, ycsb_tuple_count, FLAGS_zipf_factor);
      for (u64 t_i = 0; t_i < (FLAGS_tmp4 ? FLAGS_tmp4 : 1e6); t_i++) {
         u64 key = zipf_random->rand();
         if (ht.find(key) == ht.end()) {
            ht[key] = 0;
         } else {
            ht[key]++;
         }
      }
      csv << "key,count" << endl;
      for (auto& [key, value] : ht) {
         csv << key << "," << value << endl;
      }
      cout << ht.size() << endl;
      return 0;
   }
   // -------------------------------------------------------------------------------------
   if (FLAGS_recover) {
      // Warmup
      if (FLAGS_ycsb_warmup) {
         cout << "Warmup: Scanning..." << endl;
         {
            begin = chrono::high_resolution_clock::now();
            for (u64 i = 0; i < n; ++i) {
               YCSBPayload result;
               table.lookup1({static_cast<YCSBKey>(i)}, [&](const KVTable& record) { result = record.my_payload; });
            }
            end = chrono::high_resolution_clock::now();
         }
         // -------------------------------------------------------------------------------------
         cout << "time elapsed = " << (chrono::duration_cast<chrono::microseconds>(end - begin).count() / 1000000.0) << endl;
         cout << calculateMTPS(begin, end, n) << " M tps" << endl;
         cout << "-------------------------------------------------------------------------------------" << endl;
      }
   } else {
      cout << "Inserting " << ycsb_tuple_count << " values" << endl;
      begin = chrono::high_resolution_clock::now();
      for (u64 i = 0; i < n; ++i) {
         YCSBPayload payload;
         utils::RandomGenerator::getRandString(reinterpret_cast<u8*>(&payload), sizeof(YCSBPayload));
         YCSBKey key = i;
         table.insert({key}, {payload});
      }
      end = chrono::high_resolution_clock::now();
      cout << "time elapsed = " << (chrono::duration_cast<chrono::microseconds>(end - begin).count() / 1000000.0) << endl;
      cout << calculateMTPS(begin, end, n) << " M tps" << endl;
      // -------------------------------------------------------------------------------------
   }
   // -------------------------------------------------------------------------------------
   auto zipf_random = std::make_unique<utils::ScrambledZipfGenerator>(0, ycsb_tuple_count, FLAGS_zipf_factor);
   cout << setprecision(4);
   // -------------------------------------------------------------------------------------
   cout << "~Transactions" << endl;
   atomic<bool> keep_running = true;

   if (FLAGS_ycsb_sleepy_thread) {
      cout<<"threads not supported"<<endl;
      throw;
   }

   uint64_t tx_count=0;

   std::thread worker([&]() {
      jumpmuTry()
      begin = chrono::high_resolution_clock::now();
      while (keep_running) {
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
            ++tx_count;
         }
      }
      end=chrono::high_resolution_clock::now();
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
   cout << "-------------------------------------------------------------------------------------" << endl;
   cout << calculateMTPS(begin,end,tx_count) << " M tps" << endl;
   return 0;
}
