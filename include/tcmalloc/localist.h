#include <iostream>
#include <thread>
#include "structures.h"
#include "header.h"
#include "../stats/hoard.h"
using namespace std;

#ifndef LOCALIST
#define LOCALIST

namespace tcmalloc {

inline void addSpanToLocalList(uintptr_t span, int i, int rank) {
    // TcMallocStats::n_addSpanToLocalList++;
    // auto start = std::chrono::high_resolution_clock::now();
    uintptr_t* thread_cache = (uintptr_t*) thread_map[rank];

    list_obj* head = (list_obj*)thread_cache[i];

    // list_obj* new_head = (list_obj *)utils::mmap_(sizeof(list_obj));

    list_obj* new_head = local_buf_2[rank];

    if(new_head == NULL) {
        new_head = (list_obj *)utils::mmap_(100*sizeof(list_obj));
        auto temp = new_head;

        for(int i = 0; i < 100; i++) {
            temp->next = temp + 1;
            temp = temp->next;
        }
    }

    local_buf_2[rank] = new_head->next;


    new_head->next = head;
    new_head->addr = span;
    thread_cache[i] = (uintptr_t)new_head;

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_addSpanToLocalList += time_taken;
}


inline void initLocalThreadCache(int rank) {

    // TcMallocStats::n_initLocalThreadCache++;
    // auto start = std::chrono::high_resolution_clock::now();

    uintptr_t x = (uintptr_t)utils::mmap_(sizeof(uintptr_t)*size_classes);
    uintptr_t* thread_cache = (uintptr_t*)x;
    if(thread_cache == NULL) {
        assert("Addr is zero" && false);
    }
    thread_map[rank] = (uintptr_t) thread_cache;
    for(int i = 0 ;i < size_classes; i++) {
        thread_cache[i] = NULL_ZERO;
    }

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_initLocalThreadCache += time_taken;

}

inline uintptr_t getSpanFromLocal(int ind, int rank) {
    
    // TcMallocStats::n_getSpanFromLocal++;
    // auto start = std::chrono::high_resolution_clock::now();

    uintptr_t* thread_cache = (uintptr_t*) thread_map[rank];
    list_obj* head = (list_obj* ) thread_cache[ind];

    try {
    while(head != NULL) {


        span_meta* meta = (span_meta *)head->addr;
        auto next = head->next;
        if(meta->elems > 0) {
            auto addr = head->addr;
            // auto end = std::chrono::high_resolution_clock::now();
            // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
            // TcMallocStats::time_getSpanFromLocal += time_taken;
            return addr;
        }
        head = (list_obj *)next;
    }
    } catch(int e) {
        return NULL_ZERO;
    }

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_getSpanFromLocal += time_taken;
    return NULL_ZERO;
}

}

#endif