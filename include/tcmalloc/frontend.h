#include <vector>
class Frontend {

};

struct List{
    int sz_class;
    Block b;
    struct List* next;
};

struct Block {
    uintptr_t addr;
}

class ThreadCache {
    private:
    std::vector<List*> list(num_sz_classes, NULL);

    Block get_memory(int size) {
        if(!exists(size)) {
            // load from middle end
            addFromMiddleEnd(size);
        }

        return give_memory(size);
    }

    void addFromMiddleEnd(int size) {
        
    }

    Block pop(int size) {
        auto head = list[size];
        list[size] = head->next;
        return head->b;
    }

    Block give_memory(int size) {
        auto block = pop(size);

        if(list[size] == NULL) {
            addFromMiddleEnd(size);
        }
    }
};