#include "./include/memory.h"
#include "./include/hoard/global.h"

void testFirstFit() {
    int num_bytes = 100;
    Memory m(num_bytes);
    auto chunk1 = m.alloc(10);

    auto chunk2 = m.alloc(50);
    auto chunk3 = m.alloc(30);

    m.free(chunk2);
    m.free(chunk3);
    m.free(chunk1);
    m.printFreeList();
}

int main() {
    Hoard::AllocatorSerial alloc;
    auto b = alloc.malloc(28);
    std::cout<<b.start_ptr<<std::endl;
    std::cout<<b.super_blk<<std::endl;
    std::cout<<b.size<<std::endl;
}