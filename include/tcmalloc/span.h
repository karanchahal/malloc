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

    meta->head = (list_obj *)(linked_list_start);

    // fill in linked list
    list_obj* head = meta->head;
    list_obj* temp = head;

    for(int i = 0; i < total_elems; i++) {
        temp->addr = start_addr;
        temp->next = temp + 1;
        start_addr = start_addr + sz_class;
        temp = temp->next;
    }

    meta->linked_list_sz = header_sz;
    meta->elems = total_elems;
    meta->sz_class = sz_class;
    meta->allocated = true;

}

uintptr_t makeSpan(int num_pages) {

    uintptr_t addr = (uintptr_t)utils::mmap_(page_sz*num_pages + sizeof(span_meta));
    span_meta* meta = (span_meta *) (addr + page_sz*num_pages);
    meta->page_sz = page_sz;
    meta->num_pages = num_pages;
    meta->allocated = false;
    meta->start_addr = addr;

    return (uintptr_t) meta;
}

unordered_map<uintptr_t, uintptr_t>*  getlocalpagemap(int rank) {
    auto localmap = localmaps[rank];
    if(localmap == NULL) {
        localmaps[rank] = new unordered_map<uintptr_t, uintptr_t>;
    }
    return localmaps[rank];
}

list_obj* getlocalbuffers(int rank) {
    auto id = std::this_thread::get_id();
    auto localmap = local_buffers[rank];
    if(localmap == NULL) {
        auto obj = (list_obj*) utils::mmap_(sizeof(list_obj));
        obj->addr = NULL;
        obj->next = NULL;
        local_buffers[rank] = obj; 
    }
    return local_buffers[rank];
}

// Get span from address
uintptr_t addr2Span(uintptr_t addr, int rank) {
    uintptr_t offset = addr - addr % page_sz; // get to page offset
    auto localpagemap = getlocalpagemap(rank);
    uintptr_t span = localpagemap->at(offset);
    return span;
}


uintptr_t getSpan(int num_pages, int rank) {
    uintptr_t span = makeSpan(num_pages);
    auto localpagemap = getlocalpagemap(rank);
    addToPageMap(localpagemap, span); // add to pagemap
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

uintptr_t getObjectFromSpan(uintptr_t span, int rank) {
    span_meta* meta = (span_meta *)span;
    auto id = std::this_thread::get_id();
    if(meta->elems == 0 || meta->head == NULL) {
        // No memory available
        assert("No mem available" && false);
        return NULL; // make compiler happy
    } else {
        list_obj* new_head = meta->head->next;
        uintptr_t addr = meta->head->addr;
        list_obj* old_head = meta->head;
        meta->head = new_head;
        meta->elems--;
        auto sz = sizeof(old_head);
        auto sz2 = sizeof(list_obj);
        auto obj = getlocalbuffers(rank);
        old_head->next = obj->next;
        obj->next = old_head;
        return addr;
    }
}

}

#endif