#include "imports.h"

class Queue {
    queue<int> q;
    std::mutex mtx;

    public:

    bool empty() {
        return q.empty();
    }

    int size() {
        return q.size();
    }

    void enqueue(int i) {
        mtx.lock();
        q.push(i);
        mtx.unlock();
    }

    void dequeue() {
        mtx.lock();
        if(q.empty()) {
            mtx.unlock();
            return -1;
        }
        
        int i = q.front();
        mtx.unlock();
        return i;
    }


};