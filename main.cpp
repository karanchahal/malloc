#include "./include/memory.h"
#include "./include/hoard/serial_alloc.h"
#include "./include/hoard/parallel_alloc.h"
#include "./include/tcmalloc/main.h"
#include "./include/fasthoard/imports.h"
#include "./include/jemalloc/jemalloc.h"


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
    clock_t start, end;
    start = clock();
    for(int i = 0; i < num_times; i++) {
        malloc(32);
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

void pollTcMalloc(int num_times, int rank) {
    tcmalloc::initLocalThreadCache(rank);
    for(int i = 0; i < num_times; i++) {
       auto t = tcmalloc::alloc(32, rank);
    }
}

void pollTcMallocFree(int num_times, int rank) {
    tcmalloc::initLocalThreadCache(rank);
    for(int i = 0; i < num_times; i++) {
       auto t = tcmalloc::alloc(32, rank);
       tcmalloc::free(t,rank);
    }
}

void pollFastHoardFree(int num_times, int rank) {
    fasthoard::initNewHeap(rank);
    for(int i = 0; i < num_times; i++) {
       auto t = fasthoard::alloc(32, rank);
       fasthoard::free(t,rank);
    }
}

void pollOrigMalloc(int num_times) {
    for(int i = 0; i < num_times; i++) {
       auto v = malloc(32);
       free(v);
    }
}

void pollJEMalloc(int num_times, int index){
    for(int i = 0; i < num_times; i++) {
       auto v = jemalloc::my_malloc(32, index);
       jemalloc::my_free(v, 32);
    }
}

void testParallel(int total_calls) {
    int num_threads = 4;
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
    int num_threads = 4;
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

void testHoard() {
    testParallel(100000);
    std::cout<<"Loads done by global heap: "<<Stats::load_from_global<<std::endl;
    std::cout<<"Loads done by local heap: "<<Stats::load_from_local<<std::endl;
    auto global_heap_time = double(Stats::global_heap_access_time) / (double(Stats::load_from_global) * double(CLOCKS_PER_SEC));
    auto local_heap_time = double(Stats::local_heap_access_time) / (double(Stats::load_from_local) * double(CLOCKS_PER_SEC));

    std::cout<<"The time taken in secs by global heap access time "<<global_heap_time<<std::endl;
    std::cout<<"The time taken in secs by local heap access time "<<local_heap_time<<std::endl;

    std::cout<<"The time taken by global heap access time in clock cyles: "<<double(Stats::global_heap_access_time) / (double(Stats::load_from_global))<<std::endl;
    std::cout<<"The time taken by local heap access time in clock cyles: "<<double(Stats::local_heap_access_time) / (double(Stats::load_from_local))<<std::endl;

    std::cout<<"The factor by which global heap is slower than local heap: "<< global_heap_time / local_heap_time <<std::endl;

    std::cout<<"The number of system calls are: "<<Stats::num_sys_call<<std::endl;
    std::cout<<"The avg time taken by mmap in clock cyles: "<< double(Stats::mmap_time) / double(Stats::num_sys_call)<< std::endl;
    Stats::num_sys_call = 0;
    testThreadedSerial(100000);
     // testSerial(100000);
}

void testParallelTcMalloc(int total_calls) {
    int num_threads = 4;
    int num_each = total_calls/num_threads;
    clock_t start, end;
    vector<std::thread*> threads;

    start = clock();
    for(int i = 0; i < num_threads; i++) {
        auto t = new std::thread(pollTcMallocFree, num_each, i);
        threads.push_back(t);
    }

    for(int i = 0; i < num_threads; i++) {
        threads[i]->join();
    }

    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC);
    std::cout<<"Time taken for parallel threaded TC Malloc allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}


void testParallelFastHoard(int total_calls) {
    int num_threads = 8;
    int num_each = total_calls/num_threads;
    clock_t start, end;
    vector<std::thread*> threads;

    start = clock();
    for(int i = 0; i < num_threads; i++) {
        auto t = new std::thread(pollFastHoardFree, num_each, i);
        threads.push_back(t);
    }

    for(int i = 0; i < num_threads; i++) {
        threads[i]->join();
    }

    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC);
    std::cout<<"Time taken for parallel Hoard allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}

void testParallelOrigMalloc(int total_calls) {
    int num_threads = 8;
    int num_each = total_calls/num_threads;
    clock_t start, end;
    vector<std::thread> threads;

    start = clock();
    for(int i = 0; i < num_threads ; i++) {
        threads.push_back(std::thread(pollOrigMalloc, num_each));
    }

    for(int i = 0; i < num_threads ; i++) {
        threads[i].join();
    }
    end = clock();
    double time_taken = double(end - start) /  double(CLOCKS_PER_SEC);
    std::cout<<"Time taken for Original glib threaded TC Malloc allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}

void testParallelFastJeMalloc(int total_calls) {
  int num_threads = 4;
  int num_each = total_calls/num_threads;
  vector<std::thread*> threads;

  auto start = std::chrono::high_resolution_clock::now();
  for(int i = 0; i < num_threads; i++) {
      auto t = new std::thread(pollJEMalloc, num_each, i);
      threads.push_back(t);
  }

  for(int i = 0; i < num_threads; i++) {
      threads[i]->join();
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto time_taken = std::chrono::duration<double, std::milli>(end-start).count();
  std::cout<<"Time taken for parallel JEMalloc allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}

void testTcMalloc() {

    // testParallelTcMalloc(1000000);
    fasthoard::initGlobalHeap();
    testParallelFastHoard(1000000);
    // testParallel(1000000);
    testParallelOrigMalloc(1000000);
    // cout<<std::this_thread::get_id()<<endl;


    // tcmalloc::addToCentralList(span1);
    // tcmalloc::addToCentralList(span2);

    // tcmalloc::addToPageMap(span1);
    // uintptr_t rand_addr = span1 + 4096 + 128;
    // tcmalloc::getSpan(rand_addr);
}

void testJeMalloc() {
  jemalloc::init(10);  // num_arenas
  testParallelOrigMalloc(1000000);
  testParallelFastJeMalloc(1000000);
}

int main() {
    testJeMalloc();
}
