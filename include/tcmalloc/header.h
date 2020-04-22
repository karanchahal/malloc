#include <unordered_map>
namespace tcmalloc {


struct list_obj;
int page_sz = 4096; // 4KB
int TOTAL_PAGES = 10;
int size_classes = 5;
uintptr_t central_list[5];
unordered_map<uintptr_t, uintptr_t> pagemap;
uintptr_t pageheap[10];
unordered_map<std::thread::id, uintptr_t> thread_map;

int get_sz_class_ind(int sz) {
    switch(sz) {
        case 16: return 0;
        case 32: return 1;
        case 64: return 2;
        case 128: return 3;
        case 256: return 4;
    }

    assert(false && "You fucked up");
}

int get_nearest_size(int size) {
    if (size <= 16) {
        return 16;
    }
    if(size <= 32) {
        return 32;
    }

    if(size <= 64) {
        return 64;
    }

    if(size <= 128) {
        return 128;
    }
    return 256;
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


// Add to pagemap
void addToPageMap(uintptr_t span) {
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
    uintptr_t offset = addr - addr % page_sz; // get to page offset
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
    addToPageMap(span); // add to pagemap
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


int num_pages_needed(int size) {
    int n = size / page_sz;
    int rem = size % page_sz;
    int extra = 0;
    if (rem != 0) {
        rem = 1;
    }
    return n + rem;
}

uintptr_t popPageHeap(int i) {
    list_obj* head = (list_obj* )pageheap[i];
    list_obj *new_head = (list_obj*)head->next;
    pageheap[i] = (uintptr_t)new_head;
    return (uintptr_t)head;
}

uintptr_t splitSpan(uintptr_t span, int pgs_needed) {
    span_meta* meta = (span_meta*)span;
    int num_pgs = meta->num_pages;
    int balance_pgs = num_pgs - pgs_needed;
    meta->num_pages = pgs_needed;
    
    span_meta* new_meta = (span_meta*) utils::mmap_(sizeof(span_meta));
    new_meta->num_pages = balance_pgs;
    new_meta->head = NULL;
    new_meta->allocated = false;
    new_meta->start_addr = meta->start_addr + pgs_needed*page_sz;
    
    return (uintptr_t)new_meta;
}

uintptr_t large_alloc(int size) {
    int num_pages = num_pages_needed(size);
    cout<<num_pages<<endl;
    bool popped = false;
    uintptr_t span = NULL;
    for(int i = num_pages-1; i < TOTAL_PAGES; i++) {
        if(pageheap[i] != NULL) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }
    if(!popped) {
        span = getSpan(num_pages);
    } else {
        span_meta* meta = (span_meta *)span;
        int n_pg = meta->num_pages;
        if(n_pg > num_pages) {
            uintptr_t new_span = splitSpan(span, num_pages);
            addToPageHeap(new_span); // add to free page heap
            addToPageMap(new_span); // add to pagemap  
        }
    }

    span_meta* meta = (span_meta *)span;

    meta->allocated = true;
    return meta->start_addr;
}


int addSpanToCentralList(uintptr_t span, int i) {
    list_obj* head = (list_obj*)central_list[i];
    list_obj* new_head = (list_obj *)utils::mmap_(sizeof(list_obj*));
    new_head->addr = (uintptr_t)head;
    new_head->addr = span;
    central_list[i] = (uintptr_t)new_head;
}

uintptr_t* get_local_cache() {
    auto id = std::this_thread::get_id();
    uintptr_t* thread_cache = (uintptr_t*)thread_map[id];
    return thread_cache;
}

int addSpanToLocalList(uintptr_t span, int i) {
    uintptr_t* thread_cache = get_local_cache();

    list_obj* head = (list_obj*)thread_cache[i];
    list_obj* new_head = (list_obj *)utils::mmap_(sizeof(list_obj*));
    new_head->addr = (uintptr_t)head;
    new_head->addr = span;
    thread_cache[i] = (uintptr_t)new_head;
}

uintptr_t getObjectFromSpan(uintptr_t span) {
    span_meta* meta = (span_meta *)span;
    if(meta->elems == 0 || meta->head == NULL) {
        // No memory available
        assert("No mem available" && false);
        return NULL; // make compiler happy
    } else {
        auto new_head = (list_obj *)meta->head->next;
        uintptr_t addr = meta->head->addr;
        meta->head = new_head;
        meta->elems--;
        return addr;
    }
}


void initLocalThreadCache() {
    auto id = std::this_thread::get_id();
    if (thread_map.find(id) == thread_map.end()) {
        cout<<"init cache !"<<endl;
        // add cache
        uintptr_t* thread_cache = (uintptr_t*) utils::mmap_(sizeof(uintptr_t)*size_classes);
        thread_map[id] = (uintptr_t)thread_cache;
        for(int i = 0 ;i < size_classes; i++) {
            thread_cache[i] = 0;
        }
    }
}

uintptr_t getSpanFromLocal(int ind) {
    uintptr_t* thread_cache = get_local_cache();
    list_obj* head = (list_obj *)thread_cache[ind];
    while(head != NULL) {
        span_meta* meta = (span_meta *)head->addr;
        if(meta->elems > 0) {
            return head->addr;
        }
        head = (list_obj *)head->next;
    }

    return NULL;
}


uintptr_t getSpanFromGlobal(int ind) {
    list_obj* head = (list_obj *)central_list[ind];
    while(head != NULL) {
        span_meta* meta = (span_meta *)head->addr;
        if(meta->elems > 0) {
            return head->addr;
        }
        head = (list_obj *)head->next;
    }

    return NULL;
}


uintptr_t small_alloc(int size) {

    initLocalThreadCache();

    int sz_class = get_nearest_size(size);
    int ind = get_sz_class_ind(sz_class);
    cout<<sz_class<<endl;
    bool popped = false;
    uintptr_t span = NULL;


    // check local free list
    span = getSpanFromLocal(ind);

    if(span != NULL) {
        uintptr_t addr = getObjectFromSpan(span);
        return addr;
    }

    // check global free list
    span = getSpanFromGlobal(ind);

    // nothing found in central free list or local freelist
    for(int i = 0; i < 10; i++) {
        if(pageheap[i] != NULL) {
            span = popPageHeap(i);
            popped = true;
            break;
        }
    }
    
    int num_pages = 1;
    if(!popped) {
        cout<<"Alloc new"<<endl;
        span = getSpan(num_pages);
    }

    carveSpan(span, sz_class);
    span_meta* meta = (span_meta *)span;
    // add to thread local freelist


    addSpanToLocalList(span, ind);
    uintptr_t addr = getObjectFromSpan(span);
    return addr;
}

bool isLargeAlloc(int size) {
    if(size >= 256) {
        return true;
    }
    return false;
}

uintptr_t alloc(int size) {
    bool large_sz = isLargeAlloc(size);
    cout<<large_sz<<endl;
    if(large_sz) {
        return large_alloc(size);
    } else {
        return small_alloc(size);
    }
}

}