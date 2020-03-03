#include "imports.h"
#include "utils.h"

namespace Hoard {
class Block{
    public:
    void* start_ptr;
    int size;
    public:
    Block(void* ptr, int sz) : start_ptr(ptr), size(sz) {};
};

class SuperBlock {
    public:
    std::vector<Block> blocks;
    int size_class;
    int total_size = 4096; // size of 1 superblock
    int free_size = 4096;

    SuperBlock(int size) {
        int num_blcks = total_size/size;
        void* start_ptr = sbrk(total_size); // TODO add check for sbrk fail.

         if (start_ptr == (void*) -1) {
             assert(false && "Sbrk failed");
         }

        for(int i = 0; i < num_blcks; i++) {
            auto b = Block(start_ptr, size);
            blocks.push_back(b);
            start_ptr = (char *)start_ptr + size;
        }
    }

    void* allocBlock() {
        // TODO
        free_size -= size_class;
    }
};

class Heap {

    std::vector<SuperBlock> super_blcks;
    int total_size = 0; // memory being used
    int free_size = 0; // total memory in super block
    public:
    void add_superblock(SuperBlock &s) {
        super_blcks.push_back(s);
        total_size += s.total_size;
        free_size += s.free_size;
    }

    void alloc(int size) {
        int size_class = nearestSize(size);
        std::cout<<size_class<<std::endl;
    }
};

};
