#include "imports.h"
#include "utils.h"

namespace Hoard {
class Block{
    public:
    bool big_chunk= false;
    void* start_ptr;
    void* super_blk;
    int size;
    public:
    Block(void* ptr, int sz, void* super_blk_start) : start_ptr(ptr), size(sz), super_blk(super_blk_start) {};
    Block(void* ptr) : start_ptr(ptr), big_chunk(true) {};
};

class SuperBlock {
    public:
    std::vector<Block> blocks;
    int size_class;
    int total_size = 4096; // size of 1 superblock
    int free_size = 4096;
    int index = 0;

    SuperBlock(int size) {
        int num_blcks = total_size/size;
        void* start_ptr = utils::mmap_(total_size); // TODO add check for sbrk fail.
        void* super_blk_start = start_ptr;
        if (start_ptr == (void*) -1) {
            assert(false && "Sbrk failed");
        }

        for(int i = 0; i < num_blcks; i++) {
            auto b = Block(start_ptr, size, super_blk_start);
            blocks.push_back(b);
            start_ptr = (char *)start_ptr + size;
        }
    }

    Block get_block() {
        auto b = blocks[index];
        index += 1;
        free_size -= size_class;
        return b;
    }
};

class Heap {

    std::vector<SuperBlock*> super_blcks;
    int total_size = 0; // memory being used
    int free_size = 0; // total memory in super block
    std::unordered_map<int, std::vector<SuperBlock*>> superblk_table; // stores list of superblocks
    public:
    std::mutex mtx;

    SuperBlock* add_superblock(int size_class) {
        SuperBlock* s = new SuperBlock(size_class);
        superblk_table[size_class].push_back(s);
        total_size += s->total_size;
        free_size += s->free_size;
        return s;
    }

    SuperBlock* get_superblock(int size_class) {
        if(superblk_table.find(size_class) == superblk_table.end()) {
            auto s = new SuperBlock(size_class);
            std::vector<SuperBlock*> l;
            l.push_back(s);
            superblk_table[size_class] = l;
            return s;
        }

        auto list_blks = superblk_table[size_class];
        // get free superblocks
        int idx = -1;
        for(int i = 0; i < list_blks.size(); i++) {
            auto blk = list_blks[i];
            if(blk->free_size < size_class) {
                idx = i;
                break;
            }
        }

        if(idx == -1) return NULL;

        list_blks[idx]->free_size -= size_class; 
        return list_blks[idx];
    }


    Block alloc(int size) {
        int size_class = utils::nearestSize(size);
        auto super_blk = get_superblock(size_class);
        if(super_blk == NULL) {
            super_blk = add_superblock(size_class);
        }

        // get block from superblock
        auto blk = super_blk->get_block();
        return blk;
    }
};

class AllocatorSerial {
    int S = 4096;

    std::vector<Heap*> heaps;
    Heap* global_heap;
    public:

    AllocatorSerial() {
        global_heap = new Heap();
    }


    Block malloc(int size) {
        if(size > S/2) {
            auto addr = utils::mmap_(size);
            return Block(addr);
        }
        
        auto heap = global_heap;
        int size_class = utils::nearestSize(size);
        heap->mtx.lock();
        auto b = heap->alloc(size);
        heap->mtx.unlock();
        return b;
    }
};

};
