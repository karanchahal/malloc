#include <iostream>
#include "imports.h"
namespace tcmalloc {

void sendBackToPageHeap(span_meta* meta, uintptr_t addr) {
    meta->allocated = false;
    addToPageHeap((uintptr_t) meta);
}

list_obj* popHeadFrom(list_obj* obj) {
    auto head = obj->next;
    if(head == NULL) {
        assert(head != NULL && "There can't be 0 free buffers");
    }
    obj->next = head->next;
    return head;
}

void addAddrToSpan(span_meta* meta, uintptr_t addr, int rank) {
    auto obj = local_buffers[rank];
    if(obj->next == NULL) {
        cout<<"How the fuck !!!"<<endl;
    }
    list_obj* new_head = popHeadFrom(obj);
    new_head->addr = addr;
    new_head->next = meta->head;
    meta->head = new_head;
    meta->elems++;
}

bool shouldSendSpanBack(span_meta* meta) {
    return false;
}

void popFromLocal(span_meta* meta, int rank) {
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
        //first ele
        thread_cache[ind] = (uintptr_t)head->next;
    } else {
        prev->next = head->next;
    }
    
}

void popFromGlobal(span_meta* meta) {
    int ind = get_sz_class_ind(meta->sz_class);
    list_obj* head = (list_obj *) central_list[ind];
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
        //first ele
        central_list[ind] = (uintptr_t)head->next;
    } else {
        prev->next = head->next;
    } 
}

void sendToGlobal(span_meta* meta) {
    int ind = get_sz_class_ind(meta->sz_class);
    list_obj* head = (list_obj *) central_list[ind];
    list_obj* new_head = (list_obj *) utils::mmap_(sizeof(list_obj));
    new_head->addr = (uintptr_t)meta;
    new_head->next = head;
    central_list[ind] = (uintptr_t)new_head;
}

void free(uintptr_t addr, int rank) {
    span_meta* meta = (span_meta *) addr2Span(addr, rank);
    if(meta->elems == 0) {
        sendBackToPageHeap(meta, addr);
    } else {
        int sz_class = meta->sz_class;
        addAddrToSpan(meta, addr, rank); // adds to linked list
        
        if(shouldSendSpanBack(meta)) {
            // can send back to page heap
            // if we have enough spans in current size class.
            popFromLocal(meta, rank);

            mtx.lock();
            sendToGlobal(meta);
            // updatePageMap(meta);
            mtx.unlock();

        }
    }
}
}