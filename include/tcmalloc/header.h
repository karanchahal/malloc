#include <unordered_map>
namespace tcmalloc {


struct list_obj;
int page_sz = 4096; // 4KB
list_obj* central_list[5];
unordered_map<uintptr_t, uintptr_t> pagemap;
unordered_map<uintptr_t, uintptr_t> pageheap;

int get_sz_class(int sz) {
    switch(sz) {
        case 16: return 0;
        case 32: return 1;
        case 64: return 2;
        case 128: return 3;
    }

    assert(false && "You fucked up");
}

struct list_obj {
    uintptr_t addr;
    uintptr_t next;
};

struct span_obj {
    uintptr_t addr;
    uintptr_t next;
};

struct span_meta {
    int page_sz; // int is 4 bytes
    int sz_class;
    int num_pages;
    int linked_list_sz; // maximum linked list sz : total_elems*sizeof(list_obj)
    uintptr_t start_addr; 
    int elems; // total number of elems in linked list right now
    bool allocated;
    list_obj* head; //head of linked list
};

void carveSpan(uintptr_t span, int sz_class) {
    span_meta* meta = (span_meta *)span;
    int page_sz = meta->page_sz;
    int num_pages = meta->num_pages;

    int total_elems = page_sz*num_pages / sz_class;
    int header_sz = sizeof(list_obj)*total_elems;


    uintptr_t start_addr = (uintptr_t)utils::mmap_(header_sz);

    meta->head = (list_obj *)(start_addr);

    // fill in linked list
    list_obj* head = meta->head;
    list_obj* temp = head;

    for(int i = 0; i < total_elems; i++) {
        temp->addr = start_addr;
        temp->next = (uintptr_t)temp + sizeof(list_obj);
        start_addr = start_addr + sz_class;
        temp = (list_obj *)temp->next;
    }

    meta->linked_list_sz = header_sz;
    meta->elems = total_elems;
    meta->sz_class = sz_class;
    meta->allocated = true;

}

uintptr_t makeSpan(int num_pages) {

    // int total_elems = page_sz*num_pages / sz_class;
    // int linked_list_sz = sizeof(list_obj)*total_elems;
    // int header_sz = sizeof(span_meta) + linked_list_sz;
    // header_sz += page_sz - header_sz % page_sz; // go to next offset

    uintptr_t addr = (uintptr_t)utils::mmap_(page_sz*num_pages);
    span_meta* meta = (span_meta *) utils::mmap_(sizeof(span_meta));

    meta->page_sz = page_sz;
    meta->num_pages = num_pages;
    meta->allocated = false;
    meta->start_addr = addr;

    return (uintptr_t) meta;
}

void addToCentralList(uintptr_t span) {
    span_meta* meta = (span_meta *)span;
    int sz_class = get_sz_class(meta->sz_class);

    list_obj *new_head = (list_obj *)utils::mmap_(sizeof(list_obj));
    list_obj *obj = (list_obj *) span;
    list_obj *head = central_list[sz_class];

    new_head->addr = span;
    new_head->next = (uintptr_t)head;
    central_list[sz_class] = new_head;
}

// Add to pagemap
void addSpan(uintptr_t span) {
    span_meta* meta = (span_meta *)span;
    uintptr_t start = meta->start_addr;
    int num_pages = meta->num_pages;
    for(int i = 0; i < num_pages; i++) {
        pagemap[start] = span;
        start += meta->page_sz;
    }
}

// Get span from address
uintptr_t getSpan(uintptr_t addr) {
    uintptr_t offset = addr - addr % 4096; // get to page offset
    uintptr_t span = pagemap[offset];
    return span;
}

// Get span from address
void addToPageHeap(uintptr_t span) {
    int num_pages = ((span_meta *)span)->num_pages;
    list_obj* head = (list_obj *)pageheap[num_pages];
    list_obj *new_head = (list_obj *)utils::mmap_(sizeof(list_obj));
    new_head->addr = span;
    new_head->next = (uintptr_t)head;
    pageheap[num_pages] = (uintptr_t)new_head;
}


uintptr_t getSpan(int num_pages) {
    uintptr_t span = makeSpan(num_pages);
    addSpan(span); // add to pagemap
    return span;
}


uintptr_t getFromPageHeap(int num_pages, int sz_class) {
    bool empty = true;
    if(empty) {
        // how many spans to be allocated ?
        uintptr_t span = getSpan(num_pages);
        carveSpan(span, sz_class);
        return span;
    } else {

    }
}


uintptr_t large_alloc(int size) {
    int num_pages = num_pages_needed(size);
    bool popped = false;
    uintptr_t span = NULL;
    for(int i = num_pages; i < 10; i++) {
        if(pageheap[i] ! = NULL) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }

    if(!popped) {
        span = getSpan(num_pages);
    }

    span_meta* meta = (span_meta *)span;
    meta->allocated = true;
    return span;
}


uintptr_t small_alloc(int size) {
    int sz_class = get_nearest_size(size);
    bool popped = false;
    uintptr_t span = NULL;
    //


    // nothing found in central free list or local freelist
    for(int i = 0; i < 10; i++) {
        if(pageheap[i] ! = NULL) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }

    if(!popped) {
        span = getSpan(num_pages);
    }

    carveSpan(sz_class);

    span_meta* meta = (span_meta *)span;

    // add to thread local freelist

    return span;
}

uintptr_t alloc(int size) {
    bool large_sz = true;
    if(large_sz) {
        return large_alloc(size);
    } else {
        return small_alloc(size);
    }
}



}