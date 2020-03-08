#include "imports.h"
#ifndef BLOCK
#define BLOCK
namespace Hoard {

class Block{
    public:
    bool big_chunk= false;
    void* start_ptr;
    void* super_blk;
    int size;
    bool free = true;
    public:
    Block(void* ptr, int sz, void* super_blk_start) : start_ptr(ptr), size(sz), super_blk(super_blk_start) {};
    Block(void* ptr, int sz) : start_ptr(ptr), big_chunk(true), size(sz) {};
};

}

#endif