#ifndef SZDCUE_H_
#define SZDCUE_H_
#include <stdbool.h>
#include <stdlib.h>

#define SZQ_HEADER(TYPE, CAPACITY, SUFFIX)   \
    typedef struct {                         \
        size_t head, items, cap;             \
        int tail;                            \
        TYPE arr[CAPACITY];                  \
    } szq##SUFFIX;                           \
                                             \
    void szq##SUFFIX##_init(szq##SUFFIX *q); \
    bool szq##SUFFIX##_full(szq##SUFFIX *q); \
    void szq##SUFFIX##_pop(szq##SUFFIX *q);  \
    TYPE *szq##SUFFIX##_peek(szq##SUFFIX *q);

#define SZQ_IMPL(TYPE, CAPACITY, SUFFIX)               \
    void szq##SUFFIX##_init(szq##SUFFIX *q) {          \
        q->head = 0, q->items = 0;                     \
        q->cap = CAPACITY, q->tail = -1;               \
    }                                                  \
                                                       \
    bool szq##SUFFIX##_full(szq##SUFFIX *q) {          \
        return ((q->items) >= CAPACITY);               \
    }                                                  \
                                                       \
    void szq##SUFFIX##_push(szq##SUFFIX *q, TYPE in) { \
        ++(q->items), ++(q->tail);                     \
        q->tail -= (q->tail >= CAPACITY) * CAPACITY;   \
        (q->arr)[q->tail] = in;                        \
    }                                                  \
                                                       \
    void szq##SUFFIX##_pop(szq##SUFFIX *q) {           \
        --(q->items), ++(q->head);                     \
        q->head -= (q->head >= CAPACITY) * CAPACITY;   \
    }                                                  \
                                                       \
    TYPE *szq##SUFFIX##_peek(szq##SUFFIX *q) {         \
        return (&(q->arr)[q->head]);                   \
    }

#define SZQ(TYPE, CAP, SUFFIX)    \
    SZQ_HEADER(TYPE, CAP, SUFFIX) \
    SZQ_IMPL(TYPE, CAP, SUFFIX)

#endif
