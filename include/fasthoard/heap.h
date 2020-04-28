#include "alloc.h"
#include "utils.h"
#include "structures.h"

#ifndef FASTHOARD_HEAP
#define FASTHOARD_HEAP

namespace fasthoard {

    void add_superblock(heap* h, superblock* s) {

        // hoardStats::n_add_superblock++;
        // auto start = std::chrono::high_resolution_clock::now();

        int sz_class = s->sz_class;
        int ind = get_sz_class_ind(sz_class);
        h->total_size += s->total_size;
        h->free_size += s->free_size;
        h->blk_free_sizes[ind] += s->free_size; 

        // auto super_cover = (list_super*) utils::mmap_(sizeof(list_super)); 
        //NEW

        list_super* super_cover = h->free_list_nodes;

        if(super_cover == NULL) {
            super_cover = (list_super*) utils::mmap_(sizeof(list_super)*100);
            auto head = super_cover;
            for(int i = 0; i < 100; i++) {
                head->next = head + 1;
                head = head->next;
            }
            h->free_list_nodes = super_cover->next;

        } else {
            h->free_list_nodes = h->free_list_nodes->next;
        }

        // END NEW

        super_cover->addr = s;
        super_cover->next = h->superblks[ind];
        h->superblks[ind] = super_cover;

        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
        // hoardStats::time_add_superblock += time_taken;
    }

    // TODO: fullness groups
    superblock* get_super_from_list(heap* h, list_super* list_blks, int ind, int sz_class, bool pop=false) {
        
        // hoardStats::n_get_super_from_list++;
        // auto start = std::chrono::high_resolution_clock::now();

        list_super* prev = NULL;
        list_super* head = h->superblks[ind];
        while(head != NULL) {
            if(head->addr->free_size >= sz_class) {
                break;
            }
            prev = head;
            head = head->next;
        }

        if(head == NULL) {
            assert(false && "wtf");
        } else {
        // when superblock is found, then push to front of list
            if(prev == NULL) {
                // already first on this list, do not do anything
                h->superblks[ind] = head->next;
            } else {
                prev->next = head->next;
            }
        }

        if(!pop) {
            // add back to list, do not do this for global heap
            head->next = h->superblks[ind];
            h->superblks[ind] = head;
        } else {
            h->free_size -= head->addr->free_size;
            h->total_size -= head->addr->total_size;
        }

        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
        // hoardStats::time_get_super_from_list += time_taken;

        return head->addr;
    }


    superblock* get_superblock(heap *h, int sz_class) {
        // auto start = std::chrono::high_resolution_clock::now();

        int ind = get_sz_class_ind(sz_class);
        auto list_blks = h->superblks[ind];
        bool global = false;
        if(list_blks == NULL && h == &global_heap) {
            auto s = make_super_block(sz_class);
            return s;
        } else if(list_blks == NULL || list_blks->addr->free_size == 0) {
            global = true;
            mtx.lock();
            auto s = get_superblock(&global_heap, sz_class);
            mtx.unlock();

            add_superblock(h, s);
        }

        list_blks = h->superblks[ind];
        if(h == &global_heap) {
            auto sup = get_super_from_list(h, list_blks, ind, sz_class, true);
            return sup;
        } else {
            auto sup = get_super_from_list(h, list_blks, ind, sz_class);
            // auto end = std::chrono::high_resolution_clock::now();
            // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();

            // if(global) {
            //     hoardStats::n_global_load++;
            //     hoardStats::time_global_load += time_taken;
            // } else {
            //     hoardStats::n_local_load++;
            //     hoardStats::time_local_load += time_taken;
            // }

            return sup;
        }
    }

    void remove_superblock(heap* h, superblock* sup) {
        // hoardStats::n_remove_superblock++;
        // auto start = std::chrono::high_resolution_clock::now();

        int ind = get_sz_class_ind(sup->sz_class);
        list_super* list_blks= h->superblks[ind];
        int sz_class = sup->sz_class;

        list_super* prev = NULL;
        list_super* head = h->superblks[ind];

        // find superblock
        while(head != NULL) {
            if(head->addr == sup) {
                break;
            }
            prev = head;
            head = head->next;
        }

        // when superblock is found, the remove it from list
        if(prev == NULL) {
            h->superblks[ind] = head->next;
        } else {
            prev->next = head->next;
        }
        
        // reduce size of heap
        h->free_size -= head->addr->free_size;
        h->total_size -= head->addr->total_size;

        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
        // hoardStats::time_remove_superblock += time_taken;
    }

    void free(uintptr_t addr, int rank) {

        // auto start = std::chrono::high_resolution_clock::now();

        superblock* sup_blk = put_block(addr); // add to superblock
        heap *h = localheaps[rank]; // this won't be global
        h->free_size += sup_blk->sz_class;
        
        // Send superblock to global is lot of space available / fullness group stuff
        if(sup_blk->total_size == sup_blk->free_size && h->free_size - sup_blk->total_size > sup_blk->total_size) { // if empty
            // remove from current heap
            remove_superblock(h, sup_blk);
            // transfer over to global heap
            mtx.lock();
            add_superblock(&global_heap, sup_blk); // ADD lock here
            mtx.unlock();
            // auto end = std::chrono::high_resolution_clock::now();
            // hoardStats::n_global_free++;
            // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
            // hoardStats::time_global_free += time_taken;
        } else {
            // auto end = std::chrono::high_resolution_clock::now();
            // hoardStats::n_local_free++;
            // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
            // hoardStats::time_local_free += time_taken;
        }
    }

    uintptr_t alloc(int size, int rank) {
        int sz_class = get_nearest_size(size);
        int ind = get_sz_class_ind(sz_class);
        heap* h = localheaps[rank];
        // if found in local heap, return super else go to global heap, get it from there.
        superblock* sup = get_superblock(h, sz_class);
        h->free_size -= sz_class; // subtract from heap
        return get_block(sup); // subtract from superblk
    }

}


#endif