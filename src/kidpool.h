#ifndef KIDPOOL_H_
#define KIDPOOL_H_
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>

#include "szdcue.h"

typedef struct {
    uint8_t type;
    uint32_t info_len;
    int client;
} bcomm;

#define KIDPOOL(MAXBUCKS, MAXCUED)                         \
    SZQ(bcomm, MAXCUED, kidp)                              \
                                                           \
    typedef struct {                                       \
        int max_buckets, max_cued;                         \
        uint16_t bucks_count;                              \
        bool openbucks[MAXBUCKS];                          \
        pthread_mutex_t openb_mutex;                       \
        sem_t buck_spaces[MAXBUCKS], buck_items[MAXBUCKS]; \
        pthread_t buck_hands[MAXBUCKS];                    \
        szqkidp buck_cue[MAXBUCKS];                        \
    } kidpool;                                             \
                                                           \
    void kidpool_init(kidpool *pool) {                     \
        pool->max_buckets = MAXBUCKS;                      \
        pool->max_cued = MAXCUED;                          \
        pool->bucks_count = 0;                             \
        memset(pool->openbucks, false, MAXBUCKS);          \
        pthread_mutex_init(&(pool->openb_mutex), NULL);    \
    }                                                      \
                                                           \
    void kidpool_destroy(kidpool *pool) {                  \
        for (int i = 0; i < pool->max_buckets; ++i) {      \
            sem_destroy(&(pool->buck_spaces[i]));          \
            sem_destroy(&(pool->buck_items[i]));           \
            pthread_mutex_destroy(&(pool->openb_mutex));   \
        }                                                  \
    }

#endif
