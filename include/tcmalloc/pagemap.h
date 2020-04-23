#include <iostream>
#include "header.h"
#include "structures.h"

#ifndef PAGEMAP
#define PAGEMAP

namespace tcmalloc {

    // Add to pagemap
void addToPageMap(uintptr_t span) {
    span_meta* meta = (span_meta *)span;
    uintptr_t start = meta->start_addr;
    int num_pages = meta->num_pages;
    for(int i = 0; i < num_pages; i++) {
        pagemap[start] = span;
        start += meta->page_sz;
    }
}

}

#endif