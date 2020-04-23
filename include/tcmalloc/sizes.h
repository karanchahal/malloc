#include <iostream>
#include <assert.h>
#include "header.h"
#ifndef SIZES
#define SIZES

namespace tcmalloc {
int get_sz_class_ind(int sz) {
    switch(sz) {
        case 16: return 0;
        case 32: return 1;
        case 64: return 2;
        case 128: return 3;
        case 256: return 4;
    }

    assert(false && "You fucked up");
}

int get_nearest_size(int size) {
    if (size <= 16) {
        return 16;
    }
    if(size <= 32) {
        return 32;
    }

    if(size <= 64) {
        return 64;
    }

    if(size <= 128) {
        return 128;
    }
    return 256;
}


int num_pages_needed(int size) {
    int n = size / page_sz;
    int rem = size % page_sz;
    int extra = 0;
    if (rem != 0) {
        rem = 1;
    }
    return n + rem;
}

bool isLargeAlloc(int size) {
    if(size >= 256) {
        return true;
    }
    return false;
}

}

#endif