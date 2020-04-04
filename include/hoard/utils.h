#include "imports.h"
#include <assert.h>
#include <atomic>
#ifndef UTILS
#define UTILS

namespace Stats {
    std::atomic<int> num_sys_call;
    std::atomic<int> mmap_time;
}

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
    Stats::num_sys_call++;
    void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    assert(addr != MAP_FAILED);
    long long end = clock();
    Stats::mmap_time += (end - start);
    return addr;
}

void unmap_(void* addr, size_t size) {
    int r = munmap(addr, size);
    if(r == -1) {
        assert(false && "Munmap failed\n!");
    }
}
}

#endif