#pragma once
#include "Adapter.hpp"
// -------------------------------------------------------------------------------------
#include <btree2020.hpp>
// -------------------------------------------------------------------------------------
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

template <class Record>
struct BTreeCppAdapter final : Adapter<Record> {
   BTree* tree;
   string name;
   BTreeCppAdapter():tree(new BTree())
   {
   }
   BTreeCppAdapter(string name) : name(name),tree(new BTree()) {  }
   // -------------------------------------------------------------------------------------
   void printTreeHeight()
   {
      cout << name << " height = "
           << "unimplemented" << endl;
   }
   // -------------------------------------------------------------------------------------
   void scanDesc(const typename Record::Key& key,
                 const std::function<bool(const typename Record::Key&, const Record&)>& found_record_cb,
                 std::function<void()> reset_if_scan_failed_cb)
   {
      static u8 keyIn[Record::maxFoldLength()];
      static u8 keyOut[Record::maxFoldLength()];
      unsigned length = Record::foldKey(keyIn, key);
      typename Record::Key typedKey;
      tree->range_lookup_desc(keyIn, length, keyOut, [&](unsigned, uint8_t* payload, unsigned) {
         Record::unfoldKey(keyOut, typedKey);
         return found_record_cb(typedKey, *reinterpret_cast<const Record*>(payload));
      });
   }
   // -------------------------------------------------------------------------------------
   void insert(const typename Record::Key& key, const Record& record)
   {
      u8 k[Record::maxFoldLength()];
      u16 l = Record::foldKey(k, key);
      tree->insert(k, l, (u8*)(&record), sizeof(Record));
   }

   // -------------------------------------------------------------------------------------
   void lookup1(const typename Record::Key& key, const std::function<void(const Record&)>& cb)
   {
      u8 k[Record::maxFoldLength()];
      u16 l = Record::foldKey(k, key);
      u32 len_out;
      u8* value_ptr = tree->lookup(k, l, len_out);
      assert(value_ptr);
      cb(*reinterpret_cast<const Record*>(value_ptr));
   }


   // -------------------------------------------------------------------------------------
   void update1(const typename Record::Key& key, const std::function<void(Record&)>& cb, leanstore::UpdateSameSizeInPlaceDescriptor& update_descriptor)
   {
      u8 k[Record::maxFoldLength()];
      u16 l = Record::foldKey(k, key);
      u32 len_out;
      u8* value_ptr = tree->lookup(k, l, len_out);
      ensure(value_ptr);
      ensure(update_descriptor.count > 0);
      update_descriptor.count = 1;
      // not quite sure what offset refers to
      update_descriptor.slots[0].offset = 0;
      update_descriptor.slots[0].length = sizeof(Record);
      if (value_ptr) {
         cb(*reinterpret_cast<Record*>(value_ptr));
         tree->testing_update_payload(k, l, value_ptr);
      }
   }
   // -------------------------------------------------------------------------------------
   bool erase(const typename Record::Key& key)
   {
      u8 k[Record::maxFoldLength()];
      u16 l = Record::foldKey(k, key);
      return tree->remove(k, l);
   }
   // -------------------------------------------------------------------------------------
   void scan(const typename Record::Key& key,
             const std::function<bool(const typename Record::Key&, const Record&)>& found_record_cb,
             std::function<void()> reset_if_scan_failed_cb)
   {
      static u8 keyIn[Record::maxFoldLength()];
      static u8 keyOut[Record::maxFoldLength()];
      unsigned length = Record::foldKey(keyIn, key);
      typename Record::Key typedKey;
      tree->range_lookup(keyIn, length, keyOut, [&](unsigned, uint8_t* payload, unsigned) {
         Record::unfoldKey(keyOut, typedKey);
         return found_record_cb(typedKey, *reinterpret_cast<const Record*>(payload));
      });
   }
   // -------------------------------------------------------------------------------------
   template <class Field>
   Field lookupField(const typename Record::Key& key, Field Record::*f)
   {
      Field value;
      lookup1(key, [&](const Record& r) { value = r.*f; });
      return value;
   }
   // -------------------------------------------------------------------------------------
   u64 count()
   {
      static u8 kk[Record::maxFoldLength()];
      static u64 cnt;
      cnt = 0;
      btree_scan_asc(tree, (u8 const*)&cnt, 0, kk, [](u8 const*) {
         cnt++;
         return true;
      });
      return cnt;
   }


};
