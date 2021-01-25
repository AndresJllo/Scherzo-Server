#ifndef FSHANDLER
#define FSHANDLER

// struct for handling file system operations
// an instance of this struct can be use for each
// bucket or something
#include <stdint.h>
#include <stdio.h>

#include "fstrie.h"
#include "szdcue.h"
#define MAX_BUCKETS_FS 65536
#define MAX_FILES_FS 65536

// i dont like it but its useful for this
// might come up with a replacement

typedef uint32_t file_id;
typedef uint16_t buck_id;
typedef uint16_t file_n;

SZQ_HEADER(file_id, 65536, fid)

typedef struct {
    szqfid avail;
    uint16_t tfiles, cfiles;
    buck_id bid;
    fstrie exist;
    FILE* open[MAX_FILES_FS];
    char* prefix;
} fshandler;

file_id fsh_makeid(fshandler* fsh, file_n num);

file_n fsh_idtonum(file_id id);

void genfname(char* filepath, fshandler* fsh, int sz, file_id finfo);

void fsh_init(fshandler* fsh, int bid, const char* prefix);

bool fsh_has(file_id* finfo, fshandler* fsh, const char* name);

int fsh_rewindfp_name(fshandler* fsh, const char* filename);

int fsh_rewindfp(fshandler* fsh, file_id finfo);

int fsh_filesz(file_id* finfo, fshandler* fsh, const char* filename);

int fsh_read(file_id* finfo, void* dest, fshandler* fsh, const char* name,
             int sz);

int fsh_newfile(file_id* finfo, fshandler* fsh, const char* name);

int fsh_write(file_id* finfo, fshandler* fsh, const char* name, const void* src,
              int sz, bool replace);

int fsh_close(file_id* finfo, fshandler* fsh, const char* name);

int fsh_remove(file_id* finfo, fshandler* fsh, const char* name);

void fsh_destroy(fshandler* fsh);

#endif
