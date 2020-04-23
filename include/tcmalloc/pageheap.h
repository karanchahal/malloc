#include <iostream>
#include "header.h"
#include "span.h"
#include "../hoard/utils.h"

#ifndef PAGEHEAP
#define PAGEHEAP

namespace tcmalloc {

// Get span from address
void addToPageHeap(uintptr_t span) {
    int num_pages = ((span_meta *)span)->num_pages;
    list_obj* head = (list_obj *)pageheap[num_pages];
    list_obj *new_head = (list_obj *)utils::mmap_(sizeof(list_obj));
    new_head->addr = span;
    new_head->next = (uintptr_t)head;
    pageheap[num_pages] = (uintptr_t)new_head;
}

uintptr_t getFromPageHeap(int num_pages, int sz_class) {
    bool empty = true;
    if(empty) {
        // how many spans to be allocated ?
        uintptr_t span = getSpan(num_pages);
        carveSpan(span, sz_class);
        return span;
    } else {

    }
}

uintptr_t popPageHeap(int i) {
    list_obj* head = (list_obj* )pageheap[i];
    list_obj *new_head = (list_obj*)head->next;
    pageheap[i] = (uintptr_t)new_head;
    return (uintptr_t)head;
}


}

#endif