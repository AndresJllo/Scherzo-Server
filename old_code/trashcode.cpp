#ifndef FSTRIE
#define FSTRIE

// debugging ----------
#include <stdio.h>  //debugging

#include <iostream>
#define d(x) std::cout << #x << ' ' << x << std::endl
// end of debugging ---

#include <string.h>
// ya hate it but itll work better
// and probably faster. you can implement
// your own after the fuckin server works
#include <vector>
using std::vector;

const char ALPHABET_SIZE = 26;

template <typename T>
struct trie_node {
    int child[ALPHABET_SIZE], ch_num = 0;
    int& operator[](int index) { return child[index]; }
    bool is_leaf = false;
    T data;
    trie_node() { memset(child, -1, sizeof child); }
};

template <typename T>
struct fstrie {
    vector<trie_node<T>> nodes;
    fstrie() {
        nodes.push_back(trie_node<T>());  // push root
    }

    T& get_data(int node) { return nodes[node].data; }
    void add_str(const char* s, size_t sz, T dat) {
        int curr = 0;
        while (sz--) {
            int ch = *s - 'a';
            if (nodes[curr][ch] == -1) {
                nodes.push_back(trie_node<T>());  // create new node
                nodes[curr][ch] = nodes.size() - 1;
                nodes[curr].ch_num++;
            }
            curr = nodes[curr][ch], s++;
        }
        nodes[curr].is_leaf = true;
        nodes[curr].data = dat;
    }

    bool del_str(const char* s, size_t sz, int u = 0) {
        int ch = *s - 'a';
        if (sz == 0) return true;
        if (nodes[u][ch] != -1) {
            bool retval = del_str(s + 1, sz - 1, nodes[u][ch]);
            if(retval) nodes[u][ch] = -1;
            
        }
    }
};

#endif

