# Hoard 

A memory allocator that reduces false sharing and blowup. 

## Terminology

1. **Blowup**: Defined as the total memory used by multi threaded memory allocator divided by memory used by a ideal uniprocessor allocator.

2. **False Sharing**: This is the phenomenon when two processors share data in the same cache line. This leads to contention issues and prevents parallel operation.

3. **Superblock**: A structure with 2 objects. An array of blocks and a array of free blocks ordered in LIFO manner to get good locality of data. It is of size ```S```. The size of each block depends on what size class the superblock belongs to

4. **Local Heap**: Each thread will have a local heap that stores super blocks. It maintains statistics in the form of ```u``` and ```a```.

5. **u**: Total memory that is *live* in the heap.

6. **a**: Total memory that the heap has currently stored.

7. **Global Heap**: A heap similiar to the local heap but which is stored globally and can be accessed by all threads.

## How does false sharing happen ?

It is inevitable to an extent. One way to completly do away with flase sharing is to padd each allocation by the size of a cache line (64 bytes. This would yield a very high memory utilization which is not ideal. 

We can avoid false sharing by trying to minimize the number of processors that use data from a single cache line. Preferably have just one processor owning a cache line. False sharing can also happen when the memory allocator free's memory, this memory can then in the future be malloced by a different processor that the one who freed it. We need to avoid this as well. 

## How does Blowup happen ?

Consider threads in a producer cosumer relationship. We can get unbounded blowup when consumer requests memory- the producer ```malloc```'s it and gives it to consumer who ```free```'s it. However, this isn't reflected back to the producer. Now when new memory is needed, the producer malloc's memory again and gives it to the consumer. Hence, thememory usage can increase unbounded. A P-increase of memory is also worrying. Imagine ```P``` processors, each want memory ```m```. The total memory used is ```P*m```. If memory allocator has a P sized blowup, the total memory used will be ```P^2*m```.

# The Hoard Algorithm

1. Each heap "owns" a number of superblocks.
2. Memory requests for size larger than half of **S** are serviced by OS itself using ```mmap``` and ```munmap```.
3. Superblocks can belong to any one of **b** size classes. All blocks in a superblock belong to the same size.
4. Size classes are a power of ```b``` apart and we round up any allocation to a nearest size class. Hence, they bound worst case internal fragmentation to a factor of ```b```.
5. External fragmentation is minimized by recyclcing completely empty superblocks for re use by **any size class**. 

### Memory Allocation Policy

When memory is needed, the thread check it's local heap for memory for that size. If it not available, it refers to the global heap. A super block is transferred to the local heap if the request can be serviced. If not, the global heap asks virtual memory (the OS) for memory and forwards it to the local heap. 

**Comments**: No superblocks are returned currently to the OS in the paper version of Hoard. This seems bad ? But might be an emphirical thing, consistent with how most programs work. 

**Questions**: What block size are the new superblocks given to ? could be the size that the memory request is for rounded up to nearest power of ```b```. 

### Replacement Policy

When a local heap is more than "```f%```" empty and the local heap has more than ```K``` bytes of memory in superblocks. Hoard moves superblocks that are **at least** ```f``` empty from the local heap to the global heap. 

In other words Hoard always maintains this invariant:
``` (u >= a - K*S) or (u >= (1-f)a) ```
This is possible as when we remove a superblock, we reduce ```u``` by at most ```(1-f)*S``` (remove that much amount of live memory ) but we also reduce ```a``` by ```S``` (remove a whole super block worth of total memory). 

**Note:** A largely empty superblock is simply transferred to the global heap, it is **not freed** to the Operating system. Hence, access to some live data in a global superblock might take some time but it is still doable. 

**Comments:** One hack could be doing something clever here to prevent cache misses. Maybe modify superblock so that only completely empty superblocks are returned ? That might not be possible sometimes ?

Maintaining this bounds blowup by a constant factor.

### Information on housekeeping for Superblocks

Hoard finds ```f```-empty superblocks in constant time. This is possible by keeping bins of doubly linked lists. Each doubly linked list is ordered in a LIFO order and has superblocks of a given "fullness" range. Hoard moves superblocks from one bin to the order depening on their fullness. It always allocates from the most **full** superblocks. 

**Comment:** This could take more than constant time if not coded correctly. Say we have to assign a block of size sz, but mostly full superblocks have a lesser or far greater size. Searching for the right superblock could take time ? Maybe not if a hash table is used. 

The reason why superblocks are ordered in a LIFO manner is to improve locality of reference. If we need to allocate a block we should likely reuse a superblock that is already in memory as chances are that that it is still in the cache. 

**Note:** Is that true ? When memory is freed, is the cache freed as well ? Maybe no.

**Question:** How many superblocks of size ```x``` does local heap keep? Is there something clever we can do about this ? Having multiple objects of a lot of varied sizes could make us allocate lot of memory even as each object would demand a particular size class. I think this is a important point. Although size classes improve cache locality, it could lead to memory bloat. 

## Advantages of Hoard

Using a combination of superblocks and multiple heaps avoids active and passive false sharing.

1. Only 1 thread will allocate memory from a superblock since a superblock is owned by one heap at a given time.
2. Multiple threads allocating memory will allocate from different superblocks. Actively avoiding false sharing.
3. Deallocation of memory gives a block of memory back to the same superblock, helping avoid passive false sharing.
4. Superblock tranfer can lead to false sharing but it has not been known to occur often.
5. Often superblocks released to the global heap are completely empty. 
### Pseudo Code
```
malloc(sz) {

}

free(sz) {

}

```