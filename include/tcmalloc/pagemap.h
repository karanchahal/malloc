#include <iostream>
#include "header.h"
#include "structures.h"

#ifndef PAGEMAP
#define PAGEMAP

namespace tcmalloc {

    // Add to pagemap
void addToPageMap(unordered_map<uintptr_t,uintptr_t>* localmap, uintptr_t span) {
    // TcMallocStats::n_addToPageMap++;
    // auto starti = std::chrono::high_resolution_clock::now();

    span_meta* meta = (span_meta *)span;
    uintptr_t start = meta->start_addr;
    int num_pages = meta->num_pages;
    if(start % page_sz != 0) {
            assert("this isnt possible !" && false);
    }
    
    for(int i = 0; i < num_pages; i++) {
        localmap->insert(make_pair(start, span));
        start += meta->page_sz;
        if(start % page_sz != 0) {
            assert("this isnt possible !" && false);
        }
    }

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-starti).count();
    // TcMallocStats::time_addToPageMap += time_taken;
}

}

#endif