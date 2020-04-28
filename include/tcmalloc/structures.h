#include <iostream>

#ifndef STRUCTS
#define STRUCTS

namespace tcmalloc {

    struct list_obj {
        uintptr_t addr;
        list_obj* next;
    };

    struct span_meta {
        int page_sz; // int is 4 bytes
        int sz_class;
        int num_pages;
        int linked_list_sz; // maximum linked list sz : total_elems*sizeof(list_obj)
        uintptr_t start_addr; 
        int elems; // total number of elems in linked list right now
        bool allocated;
        list_obj* head; //head of linked list
    };

}

#endif