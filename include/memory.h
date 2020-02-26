#include <iostream>
#include <vector>

using namespace std;

struct Unit {
    int start;
    int size;

    Unit(int st, int sz) : start(st), size(sz) {};
};

class Memory {
    private:
    int total_bytes;
    vector<Unit> free_chunks;
    int total_mem_left;
    int mode; // which allocation algorithm to use

    bool overlap(Unit &one, Unit &two) {
        if(one.start + one.size == two.start) {
            return true;
        }
        return false;
    }


    public:
        Memory(int total_bytes) {
            total_bytes = total_bytes;
            mode = 1;
            total_mem_left = total_bytes;
            free_chunks.push_back(Unit(0, total_bytes));
        }

        Unit first_fit(int n) {
            if(total_mem_left < n) return Unit(-1,-1);

            int ind = -1;
            for(int i = 0; i < free_chunks.size(); i++) {
                int size = free_chunks[i].size;
                if(size >= n) {
                    ind = i;
                    break;
                }
            }

            if(ind == -1) {
                // nothing found
                return Unit(-1,-1);
            }

            auto chunk = free_chunks[ind];
            free_chunks.erase(free_chunks.begin() + ind);
            if(chunk.size == n) {
                return chunk;
            }

            Unit leftover(chunk.start + n, chunk.size - n);
            free_chunks.push_back(leftover);
            chunk.size = n;
            return chunk;
        }

        Unit alloc(int n) {
            return first_fit(n);
        }

        void free(Unit chunk) {
            free_chunks.push_back(chunk);
            compact();
        }

        // Analyse free list and intelligently merge memory chunks
        void compact() {
            sort(free_chunks.begin(), free_chunks.end(), [](Unit &one, Unit &two) {
                return one.start < two.start;
            });

            int n = free_chunks.size();
            vector<Unit> new_list;
            for(int i=0; i<n; i++) {

                if(new_list.empty()) {
                    new_list.push_back(free_chunks[i]);
                    continue;
                }

                auto curr = free_chunks[i];
                auto last = new_list[new_list.size()-1];
                if(overlap(last, curr)) {
                    new_list[new_list.size()-1].size += curr.size;
                } else {
                    new_list.push_back(curr);
                }
            }

            this->free_chunks = new_list;
        }

        void printFreeList() {
            cout<<"Printing list"<<endl;
            int n = free_chunks.size();
            for(int i = 0; i < n; i++) {
                auto curr = free_chunks[i];
                cout<<curr.start <<" "<< curr.start + curr.size<<endl;
            }
        }



};