#include <atomic>
#include <assert.h>
#include <time.h>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sys/mman.h>

using namespace std;

#ifndef HOARD_STATS
#define HOARD_STATS


namespace utils {
int nearestSize(int size) {
    /*
    Size classes:
    8, 16, 32, 64, 128, 256, 512
    */
   if(size < 0) return 0;

   if(size >= 0 && size <= 8) {
       return 8;
   } else if(size <= 16) {
       return 16;
   } else if(size <= 32) {
       return 32;
   } else if(size <= 64) {
       return 64;
   } else if(size <= 128) {
       return 128;
   } else if(size <= 256) {
       return 256;
   } else if(size <= 512) {
       return 512;
   } else {
       return -1; // need to mmap directly.
   }
}

void* mmap_(int size) {
    long long start = clock();
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    assert(addr != MAP_FAILED);
    long long end = clock();
    return addr;
}

void unmap_(void* addr, size_t size) {
    int r = munmap(addr, size);
    if(r == -1) {
        assert(false && "Munmap failed\n!");
    }
}
}

namespace TcMallocStats {
    std::atomic<int> n_lcks;
    std::atomic<long long> time_lock;

    // Time it takes to run these functions
    std::atomic<long long> time_small_alloc_local; // time it takes for mmaping 1 pages worth 4096 bytes each
    std::atomic<long long> n_small_alloc_local;

    std::atomic<long long> time_small_alloc_global; // time it takes for mmaping 1 pages worth 4096 bytes each
    std::atomic<long long> n_small_alloc_global;

    std::atomic<long long> time_getSpanFromLocal;
    std::atomic<long long> n_getSpanFromLocal;


    std::atomic<long long> time_getSpanFromGlobal;
    std::atomic<long long> n_getSpanFromGlobal;


    std::atomic<long long> time_popFromGlobal;
    std::atomic<long long> n_popFromGlobal;


    std::atomic<long long> time_sendToLocal;
    std::atomic<long long> n_sendToLocal;


    std::atomic<long long> time_popPageHeap;
    std::atomic<long long> n_popPageHeap;


    std::atomic<long long> time_getSpan;
    std::atomic<long long> n_getSpan;

    std::atomic<long long> time_carveSpan;
    std::atomic<long long> n_carveSpan;

    std::atomic<long long> time_addSpanToLocalList;
    std::atomic<long long> n_addSpanToLocalList;

    std::atomic<long long> time_getObjectFromSpan;
    std::atomic<long long> n_getObjectFromSpan;

    std::atomic<long long> time_addToPageHeap;
    std::atomic<long long> n_addToPageHeap;

    std::atomic<long long> time_addToPageMap;
    std::atomic<long long> n_addToPageMap;

    std::atomic<long long> time_splitSpan;
    std::atomic<long long> n_splitSpan;

    std::atomic<long long> time_addSpanToCentralList;
    std::atomic<long long> n_addSpanToCentralList;

    std::atomic<long long> time_sendBackToPageHeap;
    std::atomic<long long> n_sendBackToPageHeap;

    std::atomic<long long> time_shouldSendSpanBack;
    std::atomic<long long> n_shouldSendSpanBack;

    std::atomic<long long> time_popHeadFrom;
    std::atomic<long long> n_popHeadFrom;


    std::atomic<long long> time_initLocalThreadCache;
    std::atomic<long long> n_initLocalThreadCache;


    std::atomic<long long> time_getFromPageHeap;
    std::atomic<long long> n_getFromPageHeap;


    std::atomic<long long> time_makeSpan;
    std::atomic<long long> n_makeSpan;

    std::atomic<long long> time_getlocalpagemap;
    std::atomic<long long> n_getlocalpagemap;

    std::atomic<long long> time_getlocalbuffers;
    std::atomic<long long> n_getlocalbuffers;


    std::atomic<long long> time_addr2Span;
    std::atomic<long long> n_addr2Span;

    std::atomic<long long> n_getlocalbuffers_mmap;
    std::atomic<long long> time_getlocalbuffers_mmap;


    std::atomic<long long> time_sendToGlobal;
    std::atomic<long long> n_sendToGlobal;

    std::atomic<long long> time_free;
    std::atomic<long long> n_free;

    std::atomic<long long> time_popFromLocal;
    std::atomic<long long> n_popFromLocal;

    std::atomic<long long> time_addAddrToSpan;
    std::atomic<long long> n_addAddrToSpan;


}











namespace hoardStats {
    std::atomic<int> n_lcks;
    std::atomic<long long> time_lock;

    // Time it takes to run these functions
    std::atomic<long long> time_mmap_4096_1; // time it takes for mmaping 1 pages worth 4096 bytes each
    std::atomic<long long> time_mmap_4096_10; // time it takes for mmaping 10 pages worth 4096 bytes each
    std::atomic<long long> time_mmap_20_1; // time it takes for mmaping 20 bytes
    std::atomic<long long> time_mmap_16_1; // time it takes for mmaping 16 bytes
    std::atomic<long long> time_mmap_8_1; // time it takes for mmaping 8 bytes
    std::atomic<long long> time_local_heap_access;
    std::atomic<long long> time_global_heap_access;
    std::atomic<long long> time_add_superblock; // time taken by add superblock
    std::atomic<long long> time_get_super_from_list;
    std::atomic<long long> time_get_superblock;
    std::atomic<long long> time_remove_superblock;
    std::atomic<long long> time_free;
    std::atomic<long long> time_alloc;
    std::atomic<long long> time_initGlobalHeap;
    std::atomic<long long> time_initNewHeap;
    std::atomic<long long> time_put_block;
    std::atomic<long long> time_get_block;
    std::atomic<long long> time_make_super_block;


    // Number of times we access these functions
    std::atomic<long long> n_local_heap_access;
    std::atomic<long long> n_global_heap_access;
    std::atomic<long long> n_add_superblock; // time taken by add superblock
    std::atomic<long long> n_get_super_from_list;
    std::atomic<long long> n_get_superblock;
    std::atomic<long long> n_remove_superblock;
    std::atomic<long long> n_free;
    std::atomic<long long> n_alloc;
    std::atomic<long long> n_initGlobalHeap;
    std::atomic<long long> n_initNewHeap;
    std::atomic<long long> n_put_block;
    std::atomic<long long> n_get_block;
    std::atomic<long long> n_make_super_block;

    std::atomic<long long> n_global_load;
    std::atomic<long long> n_local_load;

    std::atomic<long long> time_global_load;
    std::atomic<long long> time_local_load;

    std::atomic<long long> n_local_free;
    std::atomic<long long> n_global_free;

    std::atomic<long long> time_global_free;
    std::atomic<long long> time_local_free;

    std::atomic<long long> n_nearestSize;
    std::atomic<long long> n_mmap;
}

#endif