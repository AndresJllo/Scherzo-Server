#ifndef FSHANDLER
#define FSHANDLER

// struct for handling file system operations
// an instance of this struct can be use for each
// bucket or something
#include <stdio.h>

#include <cstdint>

#include "fstrie.h"
#include "szdcue.h"
// i dont like it but its useful for this
// might come up with a replacement

typedef uint32_t file_id;
typedef uint16_t buck_id;
typedef uint16_t file_n;

// typedef char byte;
const unsigned int MAX_BUCKETS = 1 << 16, MAX_FILES = 1 << 16;




struct fshandler {
    szdcue<file_id, MAX_FILES> avail;
    uint16_t total_files = 0, curr_files = 0;
    buck_id bid;
    fstrie<file_id> exist;
    FILE* open[MAX_FILES];
    char *prefix;


    file_id makeid(file_n num) { return (bid << 16) | num; }
    buck_id bucket() { return bid; }
    uint16_t files() { return curr_files; }
    file_n idtonum(file_id id) { return (file_n)id; }

    int get_finfo(file_id* finfo, const char* name);

    int rewindfp(const char* filename);

    int readfs(file_id* finfo, const char* name, void* dest, int sz);

    int newfilefs(file_id* finfo, const char* name);

    /* writes a src buffer to a file, possibly replacing the
     * previous one.
     *
     * RETURNS: -1 if failed, bytes written otherwise
     *
     * PARAMS:
     * finfo: file_id to write file_id in
     * name: path to file, this function relies on it being null
     * terminated.
     * src: the buffer to write info from.
     * sz: the size in bytes of src, left as an int since it
     * might be used to pass a "default" -1 value.
     * replace: is the operation meant to replace or append to the
     * file?
     */
    int writefs(file_id* finfo, const char* name, const void* src, int sz,
                bool replace);

    /* closes the file
     * RETURNS: -1 if failed, 0 otherwise
     * PARAMS:
     * name: path to file
     */
    int closefs(file_id* finfo, const char* name);

    /* removes a file from the bucket
     * RETURNS: removed file id if succesful
     * PARAMS:
     * name: path to the file, relies on it being null terminated
     */
    int removefs(file_id* finfo, const char* name);

    bool has_file(file_id* finfo, const char* name);
};

#endif
