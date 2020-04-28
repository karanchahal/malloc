#include <iostream>
#include "header.h"
#include "span.h"
#include "../stats/hoard.h"

#ifndef PAGEHEAP
#define PAGEHEAP

namespace tcmalloc {

// Get span from address
void addToPageHeap(uintptr_t span) {
    // TcMallocStats::n_addToPageHeap++;
    // auto start = std::chrono::high_resolution_clock::now();

    int num_pages = ((span_meta *)span)->num_pages;
    list_obj* head = (list_obj *)pageheap[num_pages];
    list_obj *new_head = (list_obj *)utils::mmap_(sizeof(list_obj));
    new_head->addr = span;
    new_head->next = head;
    pageheap[num_pages] = new_head;

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_addToPageHeap += time_taken;
}

uintptr_t getFromPageHeap(int num_pages, int sz_class, int rank) {
    // TcMallocStats::n_getFromPageHeap++;
    // auto start = std::chrono::high_resolution_clock::now();

    bool empty = true;
    if(empty) {
        // how many spans to be allocated ?
        uintptr_t span = getSpan(num_pages, rank);
        carveSpan(span, sz_class);
        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
        // TcMallocStats::time_getFromPageHeap += time_taken;
        return span;
    } else {
        // flow not implemented as yet.
        return NULL_ZERO;
    }
}

uintptr_t popPageHeap(int i) {
    // TcMallocStats::n_popPageHeap++;
    // auto start = std::chrono::high_resolution_clock::now();

    list_obj *head = (list_obj* )pageheap[i];
    list_obj *new_head = (list_obj*)head->next;
    pageheap[i] = new_head;
    uintptr_t addr = head->addr;
    // utils::unmap_(head, sizeof(list_obj));

    // TODO: send to local buffer
    
    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_popPageHeap += time_taken;
    return addr;
}

}

#endif