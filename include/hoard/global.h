#include "imports.h"
#include "utils.h"

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

class SuperBlock {
    public:
    std::vector<Block> blocks;
    int size_class;
    int total_size = 4096; // size of 1 superblock
    int free_size = 4096;
    int index = 0;

    SuperBlock(int size) {
        int num_blcks = total_size/size;
        size_class = size;
        void* start_ptr = utils::mmap_(total_size);
        void* super_blk_start = this;

        for(int i = 0; i < num_blcks; i++) {
            auto b = Block(start_ptr, size, super_blk_start);
            blocks.push_back(b);
            start_ptr = (char *)start_ptr + size;
        }
    }

    Block get_block() {

        for(int i = 0; i < blocks.size(); i++) {
            if(blocks[i].free) {
                blocks[i].free = false;
                return blocks[i];
            }
        }
        assert(false && "ERROR: No blocks in free superblock !");
        return Block(0,0);
    }

    void free(Block &b) {
        for(int i = 0;i < this->blocks.size(); i++) {
            auto blk = this->blocks[i];
            if(blk.start_ptr == b.start_ptr) {
                this->blocks[i].free = true;
                // increase super block size
                this->free_size += this->size_class;
                return;
            }
        }
        assert(false && "ERROR: No block found to free !");
    }
};

class Heap {

    std::vector<SuperBlock*> super_blcks;
    std::unordered_map<int, std::vector<SuperBlock*>> superblk_table; // stores list of superblocks
    public:
    int free_size = 0; // total memory in super block
    int total_size = 0; // memory being used
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
            add_superblock(size_class);
        }

        auto list_blks = superblk_table[size_class];
        // get free superblocks
        int idx = -1;
        for(int i = 0; i < list_blks.size(); i++) {
            auto blk = list_blks[i];
            if(blk->free_size >= size_class) {
                idx = i;
                break;
            }
        }

        if(idx == -1) return NULL;
        auto blck = list_blks[idx];
        list_blks.erase(list_blks.begin() + idx);
        // decrease superblock size
        blck->free_size -= size_class; 
        if(blck->free_size > 0) {
            list_blks.push_back(blck);
        }
        return blck;
    }

    Block alloc(int size) {
        int size_class = utils::nearestSize(size);
        auto super_blk = get_superblock(size_class);
        // get block from superblock
        auto blk = super_blk->get_block();
        // decrease heap size
        this->free_size -= size_class;
        return blk;
    }

    void free(Block &b) {
        auto super_blk = (SuperBlock *)b.super_blk;
        super_blk->free(b);
        // increase heap size
        this->free_size += b.size;
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
            return Block(addr, size);
        }

        auto heap = global_heap;
        int size_class = utils::nearestSize(size);
        heap->mtx.lock();
        auto b = heap->alloc(size);
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

};
