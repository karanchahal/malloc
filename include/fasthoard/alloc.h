#include "utils.h"
#include "../hoard/utils.h"
#include "structures.h"

#ifndef FASTHOARD_ALLOC
#define FASTHOARD_ALLOC

namespace fasthoard {

    superblock* make_super_block(int size_class) {
        int total_eles = page_sz / size_class;
        uintptr_t start_addr = (uintptr_t) utils::mmap_(page_sz + sizeof(superblock) + sizeof(list_obj)*total_eles);
        superblock* superblk = (superblock*) (start_addr + page_sz);
        list_obj* obj = (list_obj*)(start_addr + page_sz + sizeof(superblock));
        superblk->total_size = page_sz;
        superblk->free_size = page_sz;
        superblk->size_class = size_class;
        superblk->sz_class = size_class;
        superblk->start_addr = start_addr;
        superblk->free_blcks = obj;
        superblk->free_bufs = NULL;
        // fill in linked list
        for(int i = 0; i < total_eles; i++) {
            obj->addr = start_addr;
            obj->next = obj + 1;
            obj = obj->next;
            start_addr = start_addr + size_class;
        }
        return superblk;
    }

    uintptr_t get_block(superblock* superblk) { // subtracts from superblk
        if(superblk->free_size == 0 || superblk->free_blcks == NULL) {
            assert("Error, no space in superblock" && false);
        }
        list_obj* head = superblk->free_blcks;
        auto addr = head->addr;
        auto obj = head;
        superblk->free_blcks = head->next;
        obj->next = superblk->free_bufs;
        superblk->free_bufs = obj;
        superblk->free_size -= superblk->size_class;
        return addr;
    }

    superblock* put_block(uintptr_t blk) { // ads to superblk

        auto start_addr = blk - blk%page_sz;
        // get size_cls somehow
        superblock* superblk = (superblock*) (start_addr + page_sz);
        assert(superblk->free_bufs != NULL && "Buffer has to be populated !");
        list_obj* head = superblk->free_bufs;
        superblk->free_bufs = superblk->free_bufs->next;
        head->addr = blk;
        head->next = superblk->free_blcks;
        superblk->free_blcks = head;
        superblk->free_size += superblk->size_class;
        return superblk;
    }

    void initNewHeap(int rank) {
        localheaps[rank] = (heap*) utils::mmap_(sizeof(heap));
        for(int i = 0; i < size_classes; i++) {
            localheaps[rank]->superblks[i] = NULL;
        }
    }

     void initGlobalHeap() {
        for(int i = 0; i < size_classes; i++) {
            global_heap.superblks[i] = NULL;
        }
    }

};

#endif