#include <iostream>
#include "imports.h"

#ifndef ALLOC
#define ALLOC

namespace tcmalloc {

uintptr_t large_alloc(int size) {
    int num_pages = num_pages_needed(size);
    bool popped = false;
    uintptr_t span = NULL;
    for(int i = num_pages-1; i < TOTAL_PAGES; i++) {
        if(pageheap[i] != NULL) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }
    if(!popped) {
        span = getSpan(num_pages);
    } else {
        span_meta* meta = (span_meta *)span;
        int n_pg = meta->num_pages;
        if(n_pg > num_pages) {
            uintptr_t new_span = splitSpan(span, num_pages);
            addToPageHeap(new_span); // add to free page heap
            addToPageMap(new_span); // add to pagemap  
        }
    }

    span_meta* meta = (span_meta *)span;

    meta->allocated = true;
    return meta->start_addr;
}


uintptr_t small_alloc(int size) {

    initLocalThreadCache();

    int sz_class = get_nearest_size(size);
    int ind = get_sz_class_ind(sz_class);
    bool popped = false;
    uintptr_t span = NULL;


    // check local free list
    span = getSpanFromLocal(ind);

    if(span != NULL) {
        uintptr_t addr = getObjectFromSpan(span);
        return addr;
    }

    mtx.lock();

    // check global free list
    span = getSpanFromGlobal(ind);

    // nothing found in central free list or local freelist
    for(int i = 0; i < 10; i++) {
        if(pageheap[i] != NULL) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }
    
    int num_pages = 1;
    if(!popped) {
        span = getSpan(num_pages);
    }

    carveSpan(span, sz_class);
    span_meta* meta = (span_meta *)span;
    // add to thread local freelist


    addSpanToLocalList(span, ind);
    mtx.unlock();
    
    uintptr_t addr = getObjectFromSpan(span);
    return addr;
}


uintptr_t alloc(int size) {
    bool large_sz = isLargeAlloc(size);
    if(large_sz) {
        return large_alloc(size);
    } else {
        return small_alloc(size);
    }
}

}

#endif