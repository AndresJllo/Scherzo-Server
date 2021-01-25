#ifndef FSTRIE
#define FSTRIE

//#include <iostream>
//#define d(x) std::cout << #x << ' ' << x << std::endl

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define pferr(...) fprintf(stderr, __VA_ARGS__)
const char ALPHABET_SIZE = 66;

template <typename T>
struct trie_node {
    trie_node *child[ALPHABET_SIZE];
    bool leaf = false;
    unsigned int ch_num = 0;
    T data;
    trie_node() : child() {}
    trie_node(T dat) : data(dat) {}
};

// can be further optimized later
// with stack stuff for example, return
// a different trie depending on the size
template <typename T>
struct fstrie {
   public:
    void add_str(const char *s, T dat, int sz = -1);
    bool has_str(const char *s, T *dst = nullptr, int sz = -1);
    bool del_str(const char *s, T *dst = nullptr, int sz = -1);
    void fst_destroy();

   private:
    trie_node<T> root;  // parenthesis for good measure
    bool rem_str(const char *s, size_t sz, trie_node<T> *curr, T *dst);
    bool search_str(const char *s, size_t sz, const trie_node<T> *curr, T *dst);
    void del_tree(trie_node<T> *t);
    int char_num(char a);
};

template <typename T>
int fstrie<T>::char_num(char a) {
    if (a >= '-' && a <= '9') return a - '-';
    if (a >= 'A' && a <= 'Z') return a - 'A' + 13;
    if (a == '_') return ('Z' - 'A' + 13) + 1;
    if (a >= 'a' && a <= 'z') return a - 'a' + 40;
    pferr("the char %c is not recognized\n", a);
    return -1;
}

template <typename T>
void fstrie<T>::add_str(const char *s, T dat, int sz) {
    if (sz < 0) sz = strlen(s);

    trie_node<T> *curr = &root;
    while (sz--) {
        int ch = char_num(*s);
        if ((*curr).child[ch] == nullptr) {
            (*curr).child[ch] = new trie_node<T>();
            (curr->ch_num)++;
        }
        curr = (*curr).child[ch], s++;
    }
    curr->leaf = true, curr->data = dat;
}

template <typename T>
bool fstrie<T>::has_str(const char *s, T *dst, int sz) {
    if (sz < 0) sz = strlen(s);
    return search_str(s, sz, &root, dst);
}

template <typename T>
bool fstrie<T>::search_str(const char *s, size_t sz, const trie_node<T> *curr,
                           T *dst) {
    if (sz == 0) {
        if (curr->leaf && dst != nullptr) *dst = curr->data;
        return curr->leaf;
    }
    int ch = char_num(*s);
    if (curr->child[ch] == nullptr) return false;
    return search_str(s + 1, sz - 1, curr->child[ch], dst);
}

template <typename T>
bool fstrie<T>::del_str(const char *s, T *dst, int sz) {
    if (sz < 0) sz = strlen(s);
    return rem_str(s, sz, &root, dst);
}

template <typename T>
bool fstrie<T>::rem_str(const char *s, size_t sz, trie_node<T> *curr, T *dst) {
    if (sz == 0) {
        if (curr->leaf) {
            if (dst != nullptr) *dst = curr->data;

            // if there are no children below me, return true,
            // leaf is irrelevant cuz its gonan get deleted.
            //
            // if there are children below me, return false
            // and set the leaf to false, so that it doesnt
            // show up in a query
            return curr->leaf = !(curr->ch_num > 0);
        }
        return false;  // this is not a leaf, s isnt here
    }
    int ch = char_num(*s);
    if (curr->child[ch] == nullptr) return false;

    // printf("ch %i %c\n", ch, *s);
    // trie_node<T> *child = curr->child[ch];
    // printf("leaf %i ch_num %i\n", child->leaf, child->ch_num);

    int child = rem_str(s + 1, sz - 1, curr->child[ch], dst);

    if (child == false) return false;  // cant delete below, cant delete self

    if (child == true) {  // child has nothing below it that matters
        // printf("delete %c\n", *s);
        delete curr->child[ch];
        curr->child[ch] = nullptr;
        --(curr->ch_num);
    }

    // since i've already deleted my child if it was worthless,
    // then if i have nothing else attached to me, and i am not
    // a leaf (for some other string), then i too, can die
    return (!(curr->leaf) && !(curr->ch_num > 0));
}

template <typename T>
void fstrie<T>::del_tree(trie_node<T> *t) {
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        if (t->child[i] != nullptr) {
            del_tree(t->child[i]);
            delete t->child[i];
        }
    }
}

template <typename T>
void fstrie<T>::fst_destroy() {
    del_tree(&root);
}

#endif
