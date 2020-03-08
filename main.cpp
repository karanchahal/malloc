#include "./include/memory.h"
#include "./include/hoard/serial_alloc.h"
#include "./include/hoard/parallel_alloc.h"

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
    int size = 32;
    auto b = memory->malloc(size);
    auto c = memory->malloc(size);
    auto d = memory->malloc(size);
    auto e = memory->malloc(size);
    auto f = memory->malloc(size);
    auto g = memory->malloc(size);
}

void allocParallel(Hoard::AllocatorParallel *memory) {
    int size = 32;
    auto b = memory->malloc(size);
    auto c = memory->malloc(size);
    auto d = memory->malloc(size);
    auto e = memory->malloc(size);
    auto f = memory->malloc(size);
    auto g = memory->malloc(size);
}

void testMultipleThreads(int num_threads) {
    Hoard::AllocatorSerial mem;
    vector<std::thread> threads;
    clock_t start, end; 
    start = clock();
    for(int i = 0; i < num_threads ; i++) {
        threads.push_back(std::thread(alloc, &mem));
    }
    
    for(int i = 0; i < num_threads ; i++) {
        threads[i].join();
    }
    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC); 
    std::cout<<"Time taken for sequential allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}



void testMultipleThreadsParallel(int num_threads) {
    Hoard::Heap global_heap;
    vector<std::thread> threads;
    vector<Hoard::AllocatorParallel*> mems;
    clock_t start, end; 

    for(int i = 0; i < num_threads ; i++) {
        auto mem = new Hoard::AllocatorParallel(&global_heap);
        mems.push_back(mem);
    }

    start = clock();

    for(int i = 0; i < num_threads ; i++) {
        auto mem = mems[i];
        threads.push_back(std::thread(allocParallel, mem));
    }
    
    for(int i = 0; i < num_threads ; i++) {
        threads[i].join();
    }
    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC); 
    std::cout<<"Time taken for parallel allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}

void testSerial(int num_times) {
    Hoard::AllocatorSerial mem;
    clock_t start, end; 

    start = clock();
    for(int i = 0; i < num_times; i++) {
        alloc(&mem);
    }

    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC); 
    std::cout<<"Time taken for serial no threading "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}

void simpleMemTest() {
    Hoard::AllocatorSerial alloc;
    auto b = alloc.malloc(28);
    auto c = alloc.malloc(28);
    auto d = alloc.malloc(28);
    alloc.free(b);
}

int main() {
    testSerial(500);
    testMultipleThreads(5000);
    testMultipleThreadsParallel(500);
}