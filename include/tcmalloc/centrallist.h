#include <iostream>
#include "header.h"
#include "structures.h"
#include "../hoard/utils.h"

#ifndef CENTRALIST
#define CENTRALIST

namespace tcmalloc {
int addSpanToCentralList(uintptr_t span, int i) {
    list_obj* head = (list_obj*)central_list[i];
    list_obj* new_head = (list_obj *)utils::mmap_(sizeof(list_obj*));
    new_head->addr = (uintptr_t)head;
    new_head->addr = span;
    central_list[i] = (uintptr_t)new_head;
}

uintptr_t getSpanFromGlobal(int ind) {
    list_obj* head = (list_obj *)central_list[ind];
    while(head != NULL) {
        span_meta* meta = (span_meta *)head->addr;
        auto next = head->next;
        if(meta->elems > 0) {
            auto addr = head->addr;
            return addr;
        }
        head = (list_obj *)next;
    }
    return NULL;
}

}

#endif