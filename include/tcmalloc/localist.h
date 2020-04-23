#include <iostream>
#include <thread>
#include "structures.h"
#include "header.h"
#include "../hoard/utils.h"
using namespace std;

#ifndef LOCALIST
#define LOCALIST

namespace tcmalloc {
uintptr_t* get_local_cache() {
    auto id = std::this_thread::get_id();
    uintptr_t* thread_cache = (uintptr_t*)thread_map[id];
    return thread_cache;
}

int addSpanToLocalList(uintptr_t span, int i) {
    uintptr_t* thread_cache = get_local_cache();

    list_obj* head = (list_obj*)thread_cache[i];
    list_obj* new_head = (list_obj *)utils::mmap_(sizeof(list_obj));
    new_head->next = (uintptr_t)head;
    new_head->addr = span;
    thread_cache[i] = (uintptr_t)new_head;
}


void initLocalThreadCache() {
    auto id = std::this_thread::get_id();
    if (thread_map.find(id) == thread_map.end()) {
        // add cache
        uintptr_t* thread_cache = (uintptr_t*) utils::mmap_(sizeof(uintptr_t)*size_classes);
        thread_map[id] = (uintptr_t)thread_cache;
        for(int i = 0 ;i < size_classes; i++) {
            thread_cache[i] = NULL;
        }
    }
}

uintptr_t getSpanFromLocal(int ind) {
    uintptr_t* thread_cache = get_local_cache();
    list_obj* head = (list_obj *)thread_cache[ind];
    while(head != NULL) {
        span_meta* meta = (span_meta *)head->addr;
        if(meta->elems > 0) {
            return head->addr;
        }
        head = (list_obj *)head->next;
    }

    return NULL;
}

}

#endif