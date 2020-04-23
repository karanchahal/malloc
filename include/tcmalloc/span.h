#include <iostream>
#include "../hoard/utils.h"
#include "structures.h"
#include "header.h"
#include "pagemap.h"

#ifndef SPAN
#define SPAN

namespace tcmalloc {

void carveSpan(uintptr_t span, int sz_class) {
    span_meta* meta = (span_meta *)span;
    int page_sz = meta->page_sz;
    int num_pages = meta->num_pages;

    int total_elems = page_sz*num_pages / sz_class;
    int header_sz = sizeof(list_obj)*total_elems;


    uintptr_t linked_list_start = (uintptr_t)utils::mmap_(header_sz);
    uintptr_t start_addr = meta->start_addr;
    if(linked_list_start == NULL) {
        cout<<"Fuck fuck fuck!"<<endl;
    }

    meta->head = (list_obj *)(linked_list_start);

    // fill in linked list
    list_obj* head = meta->head;
    list_obj* temp = head;

    for(int i = 0; i < total_elems; i++) {
        temp->addr = start_addr;
        temp->next = (uintptr_t)temp + sizeof(list_obj);
        start_addr = start_addr + sz_class;
        temp = (list_obj *)temp->next;
    }

    meta->linked_list_sz = header_sz;
    meta->elems = total_elems;
    meta->sz_class = sz_class;
    meta->allocated = true;

}

uintptr_t makeSpan(int num_pages) {

    // int total_elems = page_sz*num_pages / sz_class;
    // int linked_list_sz = sizeof(list_obj)*total_elems;
    // int header_sz = sizeof(span_meta) + linked_list_sz;
    // header_sz += page_sz - header_sz % page_sz; // go to next offset

    uintptr_t addr = (uintptr_t)utils::mmap_(page_sz*num_pages);
    if(addr == NULL) {
        cout<<"Fuck !"<<endl;
    }

    span_meta* meta = (span_meta *) utils::mmap_(sizeof(span_meta));
    if(meta == NULL) {
        cout<<"Holy shit !"<<endl;
    }
    meta->page_sz = page_sz;
    meta->num_pages = num_pages;
    meta->allocated = false;
    meta->start_addr = addr;

    return (uintptr_t) meta;
}

// Get span from address
uintptr_t addr2Span(uintptr_t addr) {
    uintptr_t offset = addr - addr % page_sz; // get to page offset
    uintptr_t span = pagemap[offset];
    return span;
}


uintptr_t getSpan(int num_pages) {
    uintptr_t span = makeSpan(num_pages);
    addToPageMap(span); // add to pagemap
    return span;
}

uintptr_t splitSpan(uintptr_t span, int pgs_needed) {
    span_meta* meta = (span_meta*)span;
    int num_pgs = meta->num_pages;
    int balance_pgs = num_pgs - pgs_needed;
    meta->num_pages = pgs_needed;
    
    span_meta* new_meta = (span_meta*) utils::mmap_(sizeof(span_meta));
    new_meta->num_pages = balance_pgs;
    new_meta->head = NULL;
    new_meta->allocated = false;
    new_meta->start_addr = meta->start_addr + pgs_needed*page_sz;
    
    return (uintptr_t)new_meta;
}

uintptr_t getObjectFromSpan(uintptr_t span) {
    span_meta* meta = (span_meta *)span;
    if(meta->elems == 0 || meta->head == NULL) {
        // No memory available
        assert("No mem available" && false);
        return NULL; // make compiler happy
    } else {
        list_obj* new_head = (list_obj *)meta->head->next;
        uintptr_t addr = meta->head->addr;
        meta->head = new_head;
        meta->elems--;
        return addr;
    }
}

}

#endif