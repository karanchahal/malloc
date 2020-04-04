#ifndef MAJOR_IMPORTS
#define MAJOR_IMPORTS
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/mman.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <time.h>
#include <iomanip>  //for setprecision
#include "utils.h"
#include <queue>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <atomic>
#include <algorithm>
const int SUPERBLOCK_SIZE = 4096;
namespace Stats {
std::atomic<int> load_from_global;
std::atomic<int> load_from_local;
std::atomic<int> serial_i;
std::atomic<long long> local_heap_access_time;
std::atomic<long long> global_heap_access_time;
std::atomic<long long> global_heap_time;
std::atomic<long long> local_heap_time;
};


#endif
