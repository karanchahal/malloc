#include <unordered_map>
#include <iostream>
#include "../hoard/utils.h"
using namespace std;

#ifndef FASTHOARD_STRUCT
#define FASTHOARD_STRUCT

namespace fasthoard {
const int size_classes = 5;

struct superblock;

struct list_obj {
    uintptr_t addr;
    list_obj* next;
};


struct list_super {
    superblock* addr;
    list_super* next;
};

struct superblock {
    int free_size;
    int total_size;
    int sz_class;
    int size_class;
    int index = 0;
    list_obj* free_blcks;
    uintptr_t start_addr;
    list_obj* free_bufs;
};

struct heap {
    list_super* superblks[size_classes];
    int blk_free_sizes[size_classes];
    int free_size = 0;
    int total_size = 0;
};


heap* localheaps[1000];
heap global_heap;
int page_sz = 4096; // 4KB
std::mutex mtx;

}
#endif