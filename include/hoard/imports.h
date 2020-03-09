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
// Globals
const int SUPERBLOCK_SIZE = 4096;
std::atomic<int> parallel_i;
std::atomic<int> serial_i;
#endif