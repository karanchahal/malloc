# TC Malloc


TC Malloc also known as the thread caching malloc is an allocator made by Google code master, Sanjay Ghemaway. It's primary premise is building a memory allocator that is fast in multithreaded environments. It does this by giving each thread a local memory cache requiring fewer access to a locked central memory repository.

## Thread Heap

Each thread has a local "thread heap" which stores 2 types of freelists. A large object freelist and a small object freelist. 

### The Small Object Freelist

Tt is a linked list where each index corresponds ot a particular size class. Each object corresponding to a particular size class index is a linked list of addresses of that size class. 

### The Large Object FreeList

It is a linked list too byt the index correspond to the number of pages allocated to a particular address. In short, each object in that linked list points to a linked list where each object is a page span of size index.

#### Spans

Spans are a set of continous pages. A span of size 1 is just one page and 2 is 2 continous pages and so on. 

## Global and Local Thread Heaps 

Each thread has a local thread heap with the 2 freelists and there is one global free thread heap too.

## Page Array for Spans

Each thread heap has a simple array which tracks the span to which a page belongs. Also, it track what page an address belongs to. For a 32 bit address space, we require: 2^(32) / 4096 indexes which corrspond to 4 MB of memory which is not much. For 64 bit addresses, the allocator uses a 3 level radix tree. 

The page array serves a few useful purposes. We can get the span to which a page belongs to. We have information whether a span is free or allocated. If it is free, it is present in the big object free lists (small or big freelist). If it is allocated it either:
1. Allocated as one big object to the application
2. Carved into a set of small objects and might be in pieces in the small object freelist. 


## Allocation


## Deallocation

