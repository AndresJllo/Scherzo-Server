#ifndef SZDCUE_H_
#define SZDCUE_H_

typedef struct {
    unsigned int head, items, cap;
    int tail;
} indbox;

bool full(void* raw) { 
    indbox* q = (indbox*) raw;
    return q->items >= q->cap; 
}

#define SZDCUE(TYPE, CAP, SUFFIX) \
    typedef struct {              \
        indbox inds;              \
        TYPE arr[CAP]             \
    } scue##SUFFIX;


template <typename T, const int p>
struct szdcue {
    unsigned int head = 0, items = 0;
    int tail = -1;

    T arr[p];
    bool full() { return items >= p; }
    void push(T in) {
        ++items, tail = (tail + 1) - (tail + 1 >= p) * p;
        arr[tail] = in;
    }
    void pop() { --items, head = (head + 1) - (head + 1 >= p) * p; }
    T* peek() { return &arr[head]; }
};

#endif
