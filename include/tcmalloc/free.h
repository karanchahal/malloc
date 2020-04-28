#include <iostream>
#include "imports.h"
namespace tcmalloc {

void sendBackToPageHeap(span_meta* meta, uintptr_t addr) {
    // TcMallocStats::n_sendBackToPageHeap++;
    // auto start = std::chrono::high_resolution_clock::now();
    
    meta->allocated = false;
    addToPageHeap((uintptr_t) meta);

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_sendBackToPageHeap += time_taken;
}

list_obj* popHeadFrom(list_obj* obj) {

    // TcMallocStats::n_popHeadFrom++;
    // auto start = std::chrono::high_resolution_clock::now();

    auto head = obj->next;
    if(head == NULL) {
        assert(head != NULL && "There can't be 0 free buffers");
    }
    obj->next = head->next;

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_popHeadFrom += time_taken;

    return head;
}

void addAddrToSpan(span_meta* meta, uintptr_t addr, int rank) {
    // TcMallocStats::n_addAddrToSpan++;
    //  auto start = std::chrono::high_resolution_clock::now();
    auto obj = local_buffers[rank];
    if(obj->next == NULL) {
        cout<<"How the fuck !!!"<<endl;
    }
    //  auto start = std::chrono::high_resolution_clock::now();
    list_obj* new_head = popHeadFrom(obj);
    
    new_head->addr = addr;
    new_head->next = meta->head;
    meta->head = new_head;
    meta->elems++;

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_addAddrToSpan += time_taken;

}

bool shouldSendSpanBack(span_meta* meta) {
    return false;
    if(meta->elems == ((meta->num_pages*meta->page_sz)/meta->sz_class) + 1) {
        return true;
    }
    return false;
}

void popFromLocal(span_meta* meta, int rank) {

    // TcMallocStats::n_popFromLocal++;
    // auto start = std::chrono::high_resolution_clock::now();


    uintptr_t* thread_cache = (uintptr_t *)thread_map[rank];
    int ind = get_sz_class_ind(meta->sz_class);
    list_obj* head = (list_obj *) thread_cache[ind];
    uintptr_t span = (uintptr_t)meta;
    list_obj *prev = NULL;
    while(head->addr != span && head != NULL) {
        prev = head;
        head = head->next;
    }

    if(head == NULL) {
        assert("Something is horribly wrong, should have found something" && false);
    }

    if(prev == NULL) {
        thread_cache[ind] = (uintptr_t)head->next;
    } else {
        prev->next = head->next;
    }

    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_popFromLocal += time_taken;
    
}

void sendToGlobal(span_meta* meta) {


    // TcMallocStats::n_sendToGlobal++;
    // auto start = std::chrono::high_resolution_clock::now();


    int ind = get_sz_class_ind(meta->sz_class);
    list_obj* head = (list_obj *) central_list[ind];
    list_obj* new_head = (list_obj *) utils::mmap_(sizeof(list_obj));
    new_head->addr = (uintptr_t)meta;
    new_head->next = head;
    central_list[ind] = (uintptr_t)new_head;



    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_sendToGlobal += time_taken;
}

void free(uintptr_t addr, int rank) {


    // TcMallocStats::n_free++;
    // auto start = std::chrono::high_resolution_clock::now();


    span_meta* meta = (span_meta *) addr2Span(addr, rank);
    if(meta->elems == 0) {
        //sendBackToPageHeap(meta, addr);
    } else {
        int sz_class = meta->sz_class;
        addAddrToSpan(meta, addr, rank); // adds to linked list
        
        if(shouldSendSpanBack(meta)) {
            // can send back to page heap
            // if we have enough spans in current size class.
            popFromLocal(meta, rank);

            mtx.lock();
            sendToGlobal(meta);
            mtx.unlock();
        }
    }


    // auto end = std::chrono::high_resolution_clock::now();
    // auto time_taken = std::chrono::duration<double, std::nano>(end-start).count();
    // TcMallocStats::time_free += time_taken;
}
}