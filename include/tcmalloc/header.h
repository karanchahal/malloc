#include <unordered_map>
#include <map>
#ifndef HEADER
#define HEADER

namespace tcmalloc {
struct list_obj;
int page_sz = 4096; // 4KB
int TOTAL_PAGES = 10;
int size_classes = 5;
uintptr_t central_list[5];
unordered_map<uintptr_t, uintptr_t> pagemap;
uintptr_t pageheap[10];
uintptr_t thread_map[1000]; // large value 
list_obj* local_buffers[1000];
unordered_map<uintptr_t, uintptr_t>* localmaps[1000];
std::mutex mtx;
}

#endif