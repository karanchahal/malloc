#include "imports.h"
#include "superblock.h"

#ifndef HEAP
#define HEAP

namespace Hoard {
class Heap {

    std::vector<SuperBlock*> super_blcks;
    std::unordered_map<int, std::vector<SuperBlock*>> superblk_table; // stores list of superblocks
    public:
    int free_size = 0; // total memory in super block
    int total_size = 0; // memory being used
    std::mutex mtx;

    SuperBlock* add_superblock(int size_class, bool for_local) {
        SuperBlock* s = new SuperBlock(size_class);
        if(!for_local) {
            superblk_table[size_class].push_back(s);
            total_size += s->total_size;
            free_size += s->free_size;
        }
        return s;
    }

    SuperBlock* add_superblock(SuperBlock* s, int size_class) {
        superblk_table[size_class].push_back(s);
        total_size += s->total_size;
        free_size += s->free_size;
        return s;
    }

    void pushSBToFront(int idx, int size_class, bool for_local=false) {
        auto blk = superblk_table[size_class][idx];
        superblk_table[size_class].erase(superblk_table[size_class].begin() + idx);
        if(!for_local) {
            superblk_table[size_class].push_back(blk); // do not push back if superblock is going to local heap
        }
    }

    SuperBlock* get_superblock(int size_class, bool for_local=false) {
        if(superblk_table.find(size_class) == superblk_table.end() || this->free_size < size_class) {
            auto s = add_superblock(size_class, for_local);
            if(for_local) return s;
        }
        // std::cout<<"hey"<<std::endl;
        auto list_blks = superblk_table[size_class];
        // get free superblocks
        int idx = -1;
        for(int i = 0; i < list_blks.size(); i++) {
            auto blk = list_blks[i];
            // std::cout<<this->free_size<<" "<<blk->free_size<<std::endl;
            if(blk->free_size >= size_class) {
                idx = i;
                break;
            }
        }

        if(idx == -1) {
            assert(false && "Should find a superblock of required size here !!\n");
        }

        auto blck = list_blks[idx];
        pushSBToFront(idx, size_class, for_local);
        return blck;
    }

    Block alloc(int size) {
        int size_class = utils::nearestSize(size);
        auto super_blk = get_superblock(size_class);
        auto blk = super_blk->get_block();
        this->free_size -= size_class;
        return blk;
    }

    SuperBlock* allocSuperBlock(int size) {
        int size_class = utils::nearestSize(size);
        auto super_blk = get_superblock(size_class, true);
        return super_blk;
    }

    void free(Block &b) {
        auto super_blk = (SuperBlock *)b.super_blk;
        super_blk->free(b);
        // increase heap size
        this->free_size += b.size;
    }
};
}

#endif