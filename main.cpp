#include <vector>
#include <iostream>
#include <thread>
#include "./include/stats/hoard.h"
#include "./include/tcmalloc/main.h"
#include "./include/fasthoard/imports.h"
#include "./include/jemalloc/jemalloc2.h"


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
    //    tcmalloc::free(t,rank);
    }
}



void pollFastHoardFree(int num_times, int rank) {
    fasthoard::initNewHeap(rank);
    for(int i = 0; i < num_times; i++) {
       auto t = fasthoard::alloc(32, rank);
    //    fasthoard::free(t,rank);
    }
}


void pollOrigMalloc(int num_times) {
    for(int i = 0; i < num_times; i++) {
       auto v = malloc(32);
    //    free(v);
    }
}


void testParallelTcMalloc(int total_calls, int num_threads) {
    int num_each = total_calls/num_threads;
    // clock_t start, end; 
    vector<std::thread*> threads;

    // start = clock();
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < num_threads; i++) {
        auto t = new std::thread(pollTcMallocFree, num_each, i);
        threads.push_back(t);
    }

    for(int i = 0; i < num_threads; i++) {
        threads[i]->join();
    }

    // end = clock();
    auto end = std::chrono::high_resolution_clock::now();
    auto time_taken = std::chrono::duration<double, std::milli>(end-start).count();
    std::cout<<"Time taken for parallel threaded TC Malloc allocator "<< time_taken << std::setprecision(10)<<" ms"<<std::endl;
}


void testParallelFastHoard(int total_calls, int num_threads) {
    // int num_threads = 4;
    int num_each = total_calls/num_threads;
    // clock_t start, end; 
    vector<std::thread*> threads;

    // start = clock();
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < num_threads; i++) {
        auto t = new std::thread(pollFastHoardFree, num_each, i);
        threads.push_back(t);
    }

    for(int i = 0; i < num_threads; i++) {
        threads[i]->join();
    }

    // end = clock();
    // double time_taken =  double(end - start);
    auto end = std::chrono::high_resolution_clock::now();
    auto time_taken = std::chrono::duration<double, std::milli>(end-start).count();
    std::cout<<"Time taken for parallel Hoard allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}

void testParallelOrigMalloc(int total_calls, int num_threads) {
    int num_each = total_calls/num_threads;
    // clock_t start, end; 
    auto start = std::chrono::high_resolution_clock::now();
    vector<std::thread> threads;

    // start = clock();
    for(int i = 0; i < num_threads ; i++) {
        threads.push_back(std::thread(pollOrigMalloc, num_each));
    }

    for(int i = 0; i < num_threads ; i++) {
        threads[i].join();
    }
    // end = clock();
    auto end = std::chrono::high_resolution_clock::now();
    auto time_taken = std::chrono::duration<double, std::milli>(end-start).count();
    std::cout<<"Time taken for Original glib threaded allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
}

void printHoardStats() {
    // make_super_block
    double avg_time = (double)TcMallocStats::time_sendBackToPageHeap / (double)TcMallocStats::n_sendBackToPageHeap;
    cout<<"Time by sendBackToPageHeap: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_sendBackToPageHeap<<endl;

    // make_super_block
    avg_time = (double)TcMallocStats::time_popHeadFrom / (double)TcMallocStats::n_popHeadFrom;
    cout<<"Time by popHeadFrom: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_popHeadFrom<<endl;

    avg_time = (double)TcMallocStats::time_addAddrToSpan / (double)TcMallocStats::n_addAddrToSpan;
    cout<<"Time by addAddrToSpan: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_addAddrToSpan<<endl;

    // make_super_block
     avg_time = (double)TcMallocStats::time_popFromLocal / (double)TcMallocStats::n_popFromLocal;
    cout<<"Time by popFromLocal: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_popFromLocal<<endl;


     avg_time = (double)TcMallocStats::time_addSpanToLocalList / (double)TcMallocStats::n_addSpanToLocalList;
    cout<<"Time by addSpanToLocalList: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_addSpanToLocalList<<endl;

    // make_super_block
     avg_time = (double)TcMallocStats::time_small_alloc_global / (double)TcMallocStats::n_small_alloc_global;
    cout<<"Time by small_alloc_global: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_small_alloc_global<<endl;

    // make_super_block
     avg_time = (double)TcMallocStats::time_small_alloc_local / (double)TcMallocStats::n_small_alloc_local;
    cout<<"Time by small_alloc_local: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_small_alloc_local<<endl;


    //  avg_time = (double)TcMallocStats::time_getObjectFromSpan / (double)TcMallocStats::n_getObjectFromSpan;
    // cout<<"Time by getObjectFromSpan: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_getObjectFromSpan<<endl;

    // // cout<<TcMallocStats::n_getlocalbuffers_mmap<<endl;
    // // addToPageMap

    // avg_time = (double)TcMallocStats::time_addToPageMap / (double)TcMallocStats::n_addToPageMap;
    // cout<<"Time by addToPageMap: \t"<<avg_time<<" ns\t"<<"Number of times: "<<TcMallocStats::n_addToPageMap<<endl;

}


int get_rand(int j) {
    int k = rand() % j;
    return k;
}

vector<vector<pair<int,int>>> make_random_test(int total_calls, int n_th) {
    vector<int> sizes = {16, 32, 64};
    int eles = total_calls / n_th;
    vector<vector<pair<int,int> >> ans(n_th, vector<pair<int,int>>());
    for(int i = 0; i < n_th; i++) {
        for(int j = 0;j < eles;j++) {

            pair<int,int> m; //malloc
            m.first = 0;
            m.second = sizes[get_rand(3)];

            pair<int,int> f; // free
            f.first = 1;
            f.second = m.second;

            int mode = get_rand(2);
            if(mode == 1) { // malloc at one end, free at other
                ans[i].insert(ans[i].begin(), m);
                ans[i].push_back(f);
            } else {
                ans[i].push_back(m);
                ans[i].push_back(f);
            }
        }
    }

    return ans;
}


int get_ind(int k) {
    switch(k){
        case 16: return 0;
        case 32: return 1;
        case 64: return 2;
    }
    return 0;
}

#include <stack>

void pollFastHoardFreeRand(int num_times, int rank, vector<pair<int,int>> ans) {
    fasthoard::initNewHeap(rank);
    vector<stack<uintptr_t>> addrs(3);
    for(int i = 0; i < num_times; i++) {
        if(ans[i].first == 0) {
            auto t = fasthoard::alloc(ans[i].second, rank);
            int ind = get_ind(ans[i].second);
            addrs[ind].push(t);
        } else {
            int ind = get_ind(ans[i].second);
            auto t = addrs[ind].top();
            addrs[ind].pop();
            fasthoard::free(t,rank);
        }
    }
}


void pollOrigRand(int num_times, int rank, vector<pair<int,int>> ans) {
    vector<stack<void *>> addrs(3);
    for(int i = 0; i < num_times; i++) {
        if(ans[i].first == 0) {
            auto t = malloc(ans[i].second);
            int ind = get_ind(ans[i].second);
            addrs[ind].push(t);
        } else {
            int ind = get_ind(ans[i].second);
            auto t = addrs[ind].top();
            addrs[ind].pop();
            free(t);
        }
    }
}

void pollTcMallocRand(int num_times, int rank, vector<pair<int,int>> ans) {
    tcmalloc::initLocalThreadCache(rank);
    vector<stack<uintptr_t>> addrs(3);
    for(int i = 0; i < num_times; i++) {
        if(ans[i].first == 0) {
            auto t = tcmalloc::alloc(ans[i].second, rank);
            int ind = get_ind(ans[i].second);
            addrs[ind].push(t);
        } else {
            int ind = get_ind(ans[i].second);
            auto t = addrs[ind].top();
            addrs[ind].pop();
            tcmalloc::free(t,rank);
        }
    }
}


void pollJeMallocRand(int num_times, int rank, vector<pair<int,int>> ans) {
    // tcmalloc::initLocalThreadCache(rank);
    vector<stack<void*>> addrs(3);
    for(int i = 0; i < num_times; i++) {
        if(ans[i].first == 0) {
            auto t = jemalloc::mem_malloc(ans[i].second, rank);
            int ind = get_ind(ans[i].second);
            addrs[ind].push(t);
        } else {
            int ind = get_ind(ans[i].second);
            auto t = addrs[ind].top();
            addrs[ind].pop();
            jemalloc::mem_free(t,rank);
        }
    }
}



int USE_HOARD = 0;
int USE_TCMALLOC = 1;
int USE_ORIG = 2;
int USE_JEMALLOC = 3;


void testRandParallelFastHoard(int total_calls, int num_threads, vector<vector<pair<int,int>>> &ans, int mode) {
    // int num_threads = 4;
    int num_each = total_calls/num_threads;
    // clock_t start, end; 
    vector<std::thread*> threads;

    if(mode == USE_HOARD) {
        fasthoard::initGlobalHeap();
    } else if(mode == USE_JEMALLOC) {
        jemalloc::init(16, num_threads);
    }

    // start = clock();
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < num_threads; i++) {
        if(mode == USE_HOARD) {
            auto t = new std::thread(pollFastHoardFreeRand, num_each, i, ans[i]);
            threads.push_back(t);
        } else if(mode == USE_TCMALLOC) {
            auto t = new std::thread(pollTcMallocRand, num_each, i, ans[i]);
            threads.push_back(t);
        } else if(mode == USE_ORIG) {
            auto t = new std::thread(pollOrigRand, num_each, i, ans[i]);
            threads.push_back(t);
        } else if(mode == USE_JEMALLOC) {
             auto t = new std::thread(pollJeMallocRand, num_each, i, ans[i]);
            threads.push_back(t);
        }
    }

    for(int i = 0; i < num_threads; i++) {
        threads[i]->join();
    }

    // end = clock();
    // double time_taken =  double(end - start);
    auto end = std::chrono::high_resolution_clock::now();
    auto time_taken = std::chrono::duration<double, std::milli>(end-start).count();

    if(mode == USE_HOARD) {
       std::cout<<"Time taken for parallel Hoard allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
    } else if(mode == USE_TCMALLOC) {
       std::cout<<"Time taken for parallel TCMalloc allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
    } else if(mode == USE_ORIG) {
       std::cout<<"Time taken for parallel Original allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
    } else if(mode == USE_JEMALLOC) {
        std::cout<<"Time taken for parallel JeMalloc allocator "<< time_taken << std::setprecision(10)<<" sec"<<std::endl;
    }
}





void testTcMalloc() {
    int n_th = 8;
    int prob_size = 100000;
    auto ans = make_random_test(prob_size, n_th);

    cout<<"For "<<n_th<<" NUmber of threads:"<<endl;
    testRandParallelFastHoard(prob_size, n_th, ans, USE_HOARD);

    testRandParallelFastHoard(prob_size, n_th, ans, USE_ORIG);

    testRandParallelFastHoard(prob_size, n_th, ans, USE_JEMALLOC);

    testRandParallelFastHoard(prob_size, n_th, ans, USE_TCMALLOC);

    vector<vector<pair<int,int>>> single_ans(1);

    for(int i = 0; i < n_th; i++) {
        single_ans[0].insert( single_ans[0].end(), ans[i].begin(), ans[i].end() );
        // single_ans[0] = ans[i];
    }

    cout<<"Single threaded ops"<<endl;

    testRandParallelFastHoard(prob_size, 1, single_ans, USE_HOARD);

    testRandParallelFastHoard(prob_size, 1, single_ans, USE_ORIG);

    testRandParallelFastHoard(prob_size, 1, single_ans, USE_JEMALLOC);

    testRandParallelFastHoard(prob_size, 1, single_ans, USE_TCMALLOC);
    
    // testParallelTcMalloc(100000, 4);
    // fasthoard::initGlobalHeap();
    // testParallelFastHoard(100000, 4);


    // fasthoard::initGlobalHeap();
    // testParallelFastHoard(100000, 4);


    // printHoardStats();

    // cout<<Stats::load_from_global<<" "<<Stats::load_from_local<<endl;
    // // testParallel(1000000);
    // testParallelOrigMalloc(100000, 8);
    // cout<<std::this_thread::get_id()<<endl;


    // tcmalloc::addToCentralList(span1);
    // tcmalloc::addToCentralList(span2);

    // tcmalloc::addToPageMap(span1);
    // uintptr_t rand_addr = span1 + 4096 + 128;
    // tcmalloc::getSpan(rand_addr);
}

int main() {
    testTcMalloc();
}
