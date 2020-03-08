#include "imports.h"
#include "block.h"

#ifndef SUPERBLOCK
#define SUPERBLOCK 

namespace Hoard {

class Heap;

class SuperBlock {
    public:
    std::vector<Block> blocks;
    int size_class;
    int total_size = SUPERBLOCK_SIZE; // size of 1 superblock
    int free_size = SUPERBLOCK_SIZE;
    int index = 0;
    Heap* owner;
    std::mutex mtx;

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
                this->free_size -= this->size_class;
                return blocks[i];
            }
        }
        assert(false && "ERROR: No blocks in free superblock !");
        return Block(0,0); // program will never come here
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

}

#endif