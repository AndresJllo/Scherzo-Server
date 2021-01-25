#ifndef FSTRIE
#define FSTRIE

//#include <iostream>
//#define d(x) std::cout << #x << ' ' << x << std::endl

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define pferr(...) fprintf(stderr, __VA_ARGS__)
#define ALPHABET_SIZE 66

// 67th is null terminator
static const char fst_alph[67] =
    "-./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

typedef struct trie_node_s {
    struct trie_node_s *child[ALPHABET_SIZE];
    bool leaf;
    unsigned int ch_num;
    uint32_t data;
} trie_node;

trie_node *tnode_new();

typedef struct {
    trie_node *root;
} fstrie;

void fst_init(fstrie *trie);
void fst_add(fstrie *trie, const char *s, uint32_t dat);
bool fst_has(fstrie *trie, const char *s, uint32_t *dst);
int fst_walk(fstrie *trie, int (*func)(void **), void **args, int argc);
bool fst_del(fstrie *trie, const char *s, uint32_t *dst);
void fst_destroy(fstrie *trie);

#endif
