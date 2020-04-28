#include <iostream>
#include "imports.h"

#ifndef ALLOC
#define ALLOC

namespace tcmalloc {
uintptr_t large_alloc(int size, int rank) {
    int num_pages = num_pages_needed(size);
    bool popped = false;
    uintptr_t span = NULL_ZERO;
    for(int i = num_pages-1; i < TOTAL_PAGES; i++) {
        if(pageheap[i] != NULL_ZERO) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }
    if(!popped) {
        span = getSpan(num_pages, rank);
    } else {
        span_meta* meta = (span_meta*)span;
        int n_pg = meta->num_pages;
        if(n_pg > num_pages) {
            uintptr_t new_span = splitSpan(span, num_pages);
            addToPageHeap(new_span); // add to free page heap
            addToPageMap(&pagemap, new_span); // add to pagemap  
        }
    }

    span_meta* meta = (span_meta *)span;

    meta->allocated = true;
    return meta->start_addr;
}

void popFromGlobal(span_meta* meta) {

    // TcMallocStats::n_popFromGlobal++;
    // auto start = std::chrono::high_resolution_clock::now();

    int ind = get_sz_class_ind(meta->sz_class);
    list_obj* head = (list_obj *) central_list[ind];
    uintptr_t span = (uintptr_t)meta;
    list_obj *prev = NULL_ZERO;
    while(head->addr != span && head != NULL_ZERO) {
        prev = head;
        head = head->next;
    }

    if(head == NULL_ZERO) {
        assert("Something is horribly wrong, should have found something" && false);
    }

    if(prev == NULL_ZERO) {
        //first ele
        central_list[ind] = (uintptr_t)head->next;
    } else {
        prev->next = head->next;
    }

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_popFromGlobal += time_taken;
    
}

void sendToLocal(span_meta* meta, int rank) {

    // TcMallocStats::n_sendToLocal++;
    // auto start = std::chrono::high_resolution_clock::now();


    uintptr_t* thread_cache = (uintptr_t *)thread_map[rank];
    auto local_page_map = getlocalpagemap(rank);
    int ind = get_sz_class_ind(meta->sz_class);
    list_obj* head = (list_obj *) thread_cache[ind];
    list_obj* new_head = (list_obj *) utils::mmap_(sizeof(list_obj));
    new_head->addr = (uintptr_t)meta;
    new_head->next = head;
    thread_cache[ind] = (uintptr_t)new_head;
    addToPageMap(local_page_map, (uintptr_t) meta);

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_sendToLocal += time_taken;
}

uintptr_t small_alloc(int size, int rank) {


    auto start = std::chrono::high_resolution_clock::now();


    int sz_class = get_nearest_size(size);
    int ind = get_sz_class_ind(sz_class);
    bool popped = false;
    uintptr_t span = NULL_ZERO;


    // check local free list
    span = getSpanFromLocal(ind, rank);

    if(span != NULL_ZERO) {
        uintptr_t addr = getObjectFromSpan(span, rank);
        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
        // TcMallocStats::time_small_alloc_local+= time_taken;
        // TcMallocStats::n_small_alloc_local++;
        return addr;
    }

    mtx.lock();

    // check global free list
    span = getSpanFromGlobal(ind);

    if(span != NULL_ZERO) {
        popFromGlobal((span_meta*)span);
        mtx.unlock();
        sendToLocal((span_meta*)span, rank);
        uintptr_t addr = getObjectFromSpan(span, rank);
        return addr;
    }

    // nothing found in central free list or local freelist
    for(int i = 0; i < TOTAL_PAGES; i++) {
        if(pageheap[i] != NULL) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }

    mtx.unlock();
    int num_pages = 10; // TODO: principled way of selection ?
    if(!popped) {
        span = getSpan(num_pages, rank); // has a call to global data struct
    }

    carveSpan(span, sz_class);
    span_meta* meta = (span_meta *)span;
    // add to thread local freelist

    addSpanToLocalList(span, ind, rank);

    uintptr_t addr = getObjectFromSpan(span, rank);

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_small_alloc_global+= time_taken;
    // TcMallocStats::n_small_alloc_global++;
    return addr;
}


uintptr_t alloc(int size, int rank) {
    bool large_sz = isLargeAlloc(size);
    if(large_sz) {
        return large_alloc(size, rank);
    } else {
        return small_alloc(size ,rank);
    }
}

}

#endif