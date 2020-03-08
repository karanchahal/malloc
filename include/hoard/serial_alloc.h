#include "global_heap.h"

#ifndef ALLOC_SERIAL
#define ALLOC_SERIAL

namespace Hoard {

class AllocatorSerial {
    int S = SUPERBLOCK_SIZE;

    std::vector<Heap*> heaps;
    Heap* global_heap;
    public:

    AllocatorSerial() {
        global_heap = new Heap();
    }

    ~AllocatorSerial() {
        delete global_heap;
    }

    Block malloc(int size) {
        if(size > S/2) {
            auto addr = utils::mmap_(size);
            return Block(addr, size);
        }

        auto heap = global_heap;
        int size_class = utils::nearestSize(size);
        heap->mtx.lock();
        auto b = heap->alloc(size_class);
        serial_i += 1;
        heap->mtx.unlock();
        // std::cout<<"Alloced ! Total heap size: "<<heap->free_size<<std::endl;
        return b;
    }

    void free(Block &b) {
        int size = b.size;
        if(size > S/2) {
            utils::unmap_(b.start_ptr, b.size);
            return;
        }
        auto heap = global_heap;
        heap->mtx.lock();
        heap->free(b);
        heap->mtx.unlock();
        // std::cout<<"Freed ! Total heap size: "<<heap->free_size<<std::endl;
    }

};

}

#endif