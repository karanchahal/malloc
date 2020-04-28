#include <iostream>
#include "header.h"
#include "structures.h"
#include "../stats/hoard.h"

#ifndef CENTRALIST
#define CENTRALIST

namespace tcmalloc {
void addSpanToCentralList(uintptr_t span, int i) {
    // TcMallocStats::n_addSpanToCentralList++;
    // auto start = std::chrono::high_resolution_clock::now();


    list_obj* head = (list_obj*)central_list[i];
    list_obj* new_head = (list_obj *)utils::mmap_(sizeof(list_obj*));
    new_head->addr = (uintptr_t)head;
    new_head->addr = span;
    central_list[i] = (uintptr_t)new_head;

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_addSpanToCentralList += time_taken;
}

uintptr_t getSpanFromGlobal(int ind) {
    // TcMallocStats::n_getSpanFromGlobal++;
    // auto start = std::chrono::high_resolution_clock::now();


    list_obj* head = (list_obj *)central_list[ind];
    while(head != NULL) {
        span_meta* meta = (span_meta *)head->addr;
        auto next = head->next;
        if(meta->elems > 0) {
            auto addr = head->addr;
            // auto end = std::chrono::high_resolution_clock::now();
            // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
            // TcMallocStats::time_getSpanFromGlobal += time_taken;
            return addr;
        }
        head = (list_obj *)next;
    }


    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_getSpanFromGlobal += time_taken;

    return NULL_ZERO;
}

}

#endif