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

void alloc(Hoard::AllocatorSerial *memory) {
    auto id = std::this_thread::get_id();
    std::cout<<id<<std::endl;
    int size = 32;
    auto b = memory->malloc(size);
    memory->free(b);
}

void testMultipleThreads() {
    Hoard::AllocatorSerial mem;
    int num_threads = 20000;
    vector<std::thread> threads;
    for(int i = 0; i < num_threads ; i++) {
        threads.push_back(std::thread(alloc, &mem));
    }
    
    for(int i = 0; i < num_threads ; i++) {
        threads[i].join();
    }
}

void simpleMemTest() {
    Hoard::AllocatorSerial alloc;
    auto b = alloc.malloc(28);
    auto c = alloc.malloc(28);
    auto d = alloc.malloc(28);
    alloc.free(b);
}

int main() {
    testMultipleThreads();
}