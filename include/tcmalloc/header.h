#include <unordered_map>
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
unordered_map<std::thread::id, uintptr_t> thread_map;
std::mutex mtx;
}

#endif