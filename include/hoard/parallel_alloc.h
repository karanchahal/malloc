#include "global_heap.h"

#ifndef ALLOC_PARALLEL
#define ALLOC_PARALLEL

namespace Hoard {

class AllocatorParallel {
    int S = SUPERBLOCK_SIZE;

    Heap* global_heap;
    Heap* local_heap;

    void loadFromGlobal(int size_class) {
        global_heap->mtx.lock();
        auto super_blk = global_heap->allocSuperBlock(size_class);
        global_heap->mtx.unlock();
        local_heap->add_superblock(super_blk, size_class);
    }

    public:

    AllocatorParallel(Heap* g_heap) {
        global_heap = g_heap;
        local_heap = new Heap();
    }

    Block malloc(int size) {
        if(size > S/2) {
            auto addr = utils::mmap_(size);
            return Block(addr, size);
        }

        int size_class = utils::nearestSize(size);
        if(local_heap->free_size < size_class) {
            parallel_i += 1;
            loadFromGlobal(size_class);
        }

        auto b = local_heap->alloc(size_class);
        return b;
    }

    void free(Block &b) {
        int size = b.size;
        if(size > S/2) {
            utils::unmap_(b.start_ptr, b.size);
            return;
        }
        local_heap->free(b);
        // TODO: return to global heap to reduce memry bloat.
    }

};

}

#endif