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


void testSerial(int num_times) {
    Hoard::AllocatorSerial mem;
    clock_t start, end; 

    start = clock();
    for(int i = 0; i < num_times; i++) {
        mem.malloc(32);
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


void poll(Hoard::AllocatorParallel* mem, int num_times) {
    for(int i = 0; i < num_times; i++) {
        mem->malloc(32);
    }
}


void pollSerial(Hoard::AllocatorSerial* mem, int num_times) {
    for(int i = 0; i < num_times; i++) {
        mem->malloc(32);
    }
}

void testParallel(int total_calls) {
    int num_threads = 8;
    int num_each = total_calls/num_threads;
    clock_t start, end; 
    Hoard::Heap global_heap;
    vector<Hoard::AllocatorParallel*> mems;
    vector<std::thread> threads;

    for(int i = 0; i < num_threads ; i++) {
        auto mem = new Hoard::AllocatorParallel(&global_heap);
        mems.push_back(mem);
    }
    
    start = clock();
    for(int i = 0; i < num_threads ; i++) {
        auto mem = mems[i];
        threads.push_back(std::thread(poll, mem, num_each));
    }

    for(int i = 0; i < num_threads ; i++) {
        threads[i].join();
    }
    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC); 
    std::cout<<"Time taken for parallel threaded allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;

}



void testThreadedSerial(int total_calls) {
    int num_threads = 8;
    int num_each = total_calls/num_threads;
    clock_t start, end; 
    Hoard::AllocatorSerial mem;
    vector<std::thread> threads;
    
    start = clock();
    for(int i = 0; i < num_threads ; i++) {
        threads.push_back(std::thread(pollSerial, &mem, num_each));
    }

    for(int i = 0; i < num_threads ; i++) {
        threads[i].join();
    }

    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC); 
    std::cout<<"Time taken for serial threaded allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;

}

int main() {

    testParallel(100000);
    testThreadedSerial(50000);
    testSerial(100000);
}
