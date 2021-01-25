#include "fstrie.h"

/*HELPER FUNCTIONS-----------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int fst_charnum(char a) {
    if (a >= '-' && a <= '9') return a - '-';
    if (a >= 'A' && a <= 'Z') return a - 'A' + 13;
    if (a == '_') return ('Z' - 'A' + 13) + 1;
    if (a >= 'a' && a <= 'z') return a - 'a' + 40;
    pferr("the char %c is not recognized\n", a);
    return -1;
}

bool search_str(const char *s, size_t sz, const trie_node *curr,
                uint32_t *dst) {
    if (sz == 0) {
        if (curr->leaf && dst != NULL) *dst = curr->data;
        return curr->leaf;
    }
    int ch = fst_charnum(*s);
    if (curr->child[ch] == NULL) return false;
    return search_str(s + 1, sz - 1, curr->child[ch], dst);
}

bool rem_str(const char *s, size_t sz, trie_node *curr, uint32_t *dst) {
    if (sz == 0) {
        if (curr->leaf) {
            if (dst != NULL) *dst = curr->data;
            // no children: leaf = true, deletes
            // children: leaf = false, remains
            return curr->leaf = !(curr->ch_num > 0);
        }
        return false;
    }
    int ch = fst_charnum(*s);
    if (curr->child[ch] == NULL) return false;

    int child = rem_str(s + 1, sz - 1, curr->child[ch], dst);
    if (child == false) return false;
    free(curr->child[ch]);
    curr->child[ch] = NULL;
    --(curr->ch_num);

    return (!(curr->leaf) && !(curr->ch_num > 0));
}

int walk_trie(char *buf, int *offset, trie_node *curr, int (*func)(void **),
              void **args) {
    args[2] = curr;
    if (curr->leaf && func(args) == -1) return -1;

    for (int ch = 0; ch < ALPHABET_SIZE; ++ch) {
        if (curr->child[ch] != NULL) {
            buf[*offset] = fst_alph[ch];
            ++(*offset);
            int worked = walk_trie(buf, offset, curr->child[ch], func, args);
            --(*offset);
            buf[*offset] = '\0';

            if (worked == -1) return -1;
        }
    }
    return 0;
}

void del_tree(trie_node *t) {
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        if (t->child[i] != NULL) {
            del_tree(t->child[i]);
            free(t->child[i]);
        }
    }
}
/*HELPER FUNCTIONS END-------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

trie_node *tnode_new() {
    trie_node *node = (trie_node *)malloc(sizeof(trie_node));

    node->leaf = false;
    node->ch_num = 0;
    node->data = 0;
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        (node->child)[i] = NULL;
    }

    return node;
}

void fst_init(fstrie *trie) { trie->root = tnode_new(); }

void fst_add(fstrie *trie, const char *s, uint32_t dat) {
    int sz = strlen(s);

    trie_node *curr = trie->root;
    while (sz--) {
        int ch = fst_charnum(*s);
        if ((curr->child)[ch] == NULL) {
            (curr->child)[ch] = tnode_new();
            ++(curr->ch_num);
        }
        curr = (curr->child)[ch], ++s;
    }
    curr->leaf = true, curr->data = dat;
}

bool fst_has(fstrie *trie, const char *s, uint32_t *dst) {
    int sz = strlen(s);
    return search_str(s, sz, trie->root, dst);
}

int fst_walk(fstrie *trie, int (*func)(void **), void **args, int argc) {
    char buf[202];
    memset(buf, 0, sizeof buf);
    int offset = 0;

    void *new_args[argc + 3];
    new_args[0] = buf;
    new_args[1] = &offset;
    new_args[2] = NULL;  // where the node will be
    for (int i = 0; i < argc; ++i) new_args[i + 3] = args[i];

    return walk_trie(buf, &offset, trie->root, func, new_args);
}

bool fst_del(fstrie *trie, const char *s, uint32_t *dst) {
    int sz = strlen(s);
    return rem_str(s, sz, trie->root, dst);
}

void fst_destroy(fstrie *trie) {
    del_tree(trie->root);
    free(trie->root);
}
