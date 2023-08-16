add_executable(frontend frontend.cpp)
add_dependencies(frontend leanstore)
target_link_libraries(frontend leanstore)

add_executable(bulk_loading mix/bulk_loading.cpp)
target_link_libraries(bulk_loading leanstore Threads::Threads)
target_include_directories(bulk_loading PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(ycsb ycsb/ycsb.cpp)
target_link_libraries(ycsb leanstore Threads::Threads)
target_include_directories(ycsb PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(deterministic ycsb/deterministic.cpp)
target_link_libraries(deterministic leanstore Threads::Threads)
target_include_directories(deterministic PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(tpcc tpc-c/tpcc.cpp)
target_link_libraries(tpcc leanstore Threads::Threads)
target_include_directories(tpcc PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(graveyard micro-benchmarks/graveyard.cpp)
target_link_libraries(graveyard leanstore Threads::Threads)
target_include_directories(graveyard PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(queue micro-benchmarks/queue.cpp)
target_link_libraries(queue leanstore Threads::Threads)
target_include_directories(queue PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(min minimalExample/main.cpp)
target_link_libraries(min leanstore Threads::Threads)
target_include_directories(min PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(rocksdb_tpcc tpc-c/rocksdb_tpcc.cpp)
target_link_libraries(rocksdb_tpcc leanstore rocksdb Threads::Threads dl z libbz2.a lz4 snappy zstd uring)
target_include_directories(rocksdb_tpcc PRIVATE ${SHARED_INCLUDE_DIRECTORY})
target_compile_definitions(rocksdb_tpcc PUBLIC ROCKSDB_ADAPTER)

add_executable(rocksdb_ycsb ycsb/rocksdb_ycsb.cpp)
target_link_libraries(rocksdb_ycsb leanstore rocksdb Threads::Threads dl z libbz2.a lz4 snappy zstd uring)
target_include_directories(rocksdb_ycsb PRIVATE ${SHARED_INCLUDE_DIRECTORY})
target_compile_definitions(rocksdb_ycsb PUBLIC ROCKSDB_ADAPTER)
# sudo apt-get install libsnappy-dev  zlib1g-dev libbz2-dev liblz4-dev libzstd-dev librocksdb-dev

add_executable(wiredtiger_tpcc tpc-c/wiredtiger_tpcc.cpp)
target_link_libraries(wiredtiger_tpcc leanstore wiredtiger Threads::Threads dl z libbz2.a lz4 snappy)
target_include_directories(wiredtiger_tpcc PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(wiredtiger_ycsb ycsb/wiredtiger_ycsb.cpp)
target_link_libraries(wiredtiger_ycsb leanstore wiredtiger Threads::Threads dl z libbz2.a lz4 snappy)
target_include_directories(wiredtiger_ycsb PRIVATE ${SHARED_INCLUDE_DIRECTORY})
# prefer https://source.wiredtiger.com/10.0.0/build-posix.html over sudo apt-get install wiredtiger libwiredtiger-dev


add_executable(lmdb_tpcc tpc-c/lmdb_tpcc.cpp)
target_link_libraries(lmdb_tpcc leanstore lmdb Threads::Threads)
target_include_directories(lmdb_tpcc PRIVATE ${SHARED_INCLUDE_DIRECTORY})

add_executable(lmdb_ycsb ycsb/lmdb_ycsb.cpp)
target_link_libraries(lmdb_ycsb leanstore lmdb Threads::Threads)
target_include_directories(lmdb_ycsb PRIVATE ${SHARED_INCLUDE_DIRECTORY})
# sudo apt-get install liblmdb-dev


# btree-cpp
set (btree_cpp_ycsb_sources ycsb/btree_cpp_ycsb.cpp ../../btree-cpp/btree/anynode.cpp  ../../btree-cpp/btree/btree2020.cpp  ../../btree-cpp/btree/debug.cpp  ../../btree-cpp/btree/dense.cpp  ../../btree-cpp/btree/hash.cpp)

add_executable(btree_cpp_ycsb ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/btree/")

## btree-cpp named builds
add_executable(btree_cpp_ycsb_baseline ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb_baseline leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb_baseline PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/named-configs/" "../../btree-cpp/btree/")
target_compile_options(btree_cpp_ycsb_baseline PRIVATE -include baseline.hpp -DNAMED_CONFIG=\"baseline\")

add_executable(btree_cpp_ycsb_prefix ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb_prefix leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb_prefix PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/named-configs/" "../../btree-cpp/btree/")
target_compile_options(btree_cpp_ycsb_prefix PRIVATE -include prefix.hpp -DNAMED_CONFIG=\"prefix\")

add_executable(btree_cpp_ycsb_heads ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb_heads leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb_heads PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/named-configs/" "../../btree-cpp/btree/")
target_compile_options(btree_cpp_ycsb_heads PRIVATE -include heads.hpp -DNAMED_CONFIG=\"heads\")

add_executable(btree_cpp_ycsb_hints ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb_hints leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb_hints PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/named-configs/" "../../btree-cpp/btree/")
target_compile_options(btree_cpp_ycsb_hints PRIVATE -include hints.hpp -DNAMED_CONFIG=\"hints\")

add_executable(btree_cpp_ycsb_hash ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb_hash leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb_hash PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/named-configs/" "../../btree-cpp/btree/")
target_compile_options(btree_cpp_ycsb_hash PRIVATE -include hash.hpp -DNAMED_CONFIG=\"hash\")

add_executable(btree_cpp_ycsb_inner ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb_inner leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb_inner PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/named-configs/" "../../btree-cpp/btree/")
target_compile_options(btree_cpp_ycsb_inner PRIVATE -include inner.hpp -DNAMED_CONFIG=\"inner\")

add_executable(btree_cpp_ycsb_dense ${btree_cpp_ycsb_sources})
target_link_libraries(btree_cpp_ycsb_dense leanstore Threads::Threads)
target_include_directories(btree_cpp_ycsb_dense PRIVATE ${SHARED_INCLUDE_DIRECTORY} "../../btree-cpp/named-configs/" "../../btree-cpp/btree/")
target_compile_options(btree_cpp_ycsb_dense PRIVATE -include dense.hpp -DNAMED_CONFIG=\"dense\")
