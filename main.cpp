#include "./include/memory.h"

int main() {
    int num_bytes = 100;
    Memory m(num_bytes);
    auto chunk1 = m.alloc(10);

    auto chunk2 = m.alloc(50); // 10 + 50: 60
    auto chunk3 = m.alloc(30);

    m.free(chunk2);
    m.free(chunk3);
    m.free(chunk1);
    m.printFreeList();
   
}