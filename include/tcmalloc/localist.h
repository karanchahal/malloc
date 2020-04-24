#include <iostream>
#include <thread>
#include "structures.h"
#include "header.h"
#include "../hoard/utils.h"
using namespace std;

#ifndef LOCALIST
#define LOCALIST

namespace tcmalloc {

inline int addSpanToLocalList(uintptr_t span, int i, int rank) {
    uintptr_t* thread_cache = (uintptr_t*) thread_map[rank];

    list_obj* head = (list_obj*)thread_cache[i];
    list_obj* new_head = (list_obj *)utils::mmap_(sizeof(list_obj));
    new_head->next = head;
    new_head->addr = span;
    thread_cache[i] = (uintptr_t)new_head;
}


inline void initLocalThreadCache(int rank) {

    uintptr_t x = (uintptr_t)utils::mmap_(sizeof(uintptr_t)*size_classes);
    uintptr_t* thread_cache = (uintptr_t*)x;
    if(thread_cache == NULL) {
        assert("Addr is zero" && false);
    }
    thread_map[rank] = (uintptr_t) thread_cache;
    for(int i = 0 ;i < size_classes; i++) {
        thread_cache[i] = NULL;
    }
}

inline uintptr_t getSpanFromLocal(int ind, int rank) {
    uintptr_t* thread_cache = (uintptr_t*) thread_map[rank];
    list_obj* head = (list_obj* ) thread_cache[ind];
    while(head != NULL) {
        span_meta* meta = (span_meta *)head->addr;
        auto next = head->next;
        if(meta->elems > 0) {
            auto addr = head->addr;
            return addr;
        }
        head = (list_obj *)next;
    }

    return NULL;
}

}

#endif