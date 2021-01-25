#include "fshandler.h"

#include <stdio.h>

SZQ_IMPL(file_id, 65536, fid)

// fshandler-------------------------------
// TODO: make this constructor populate exist and
// open accordingly

void genfname(char *filepath, fshandler *fsh, int sz, file_id finfo) {
    int offset = 0;
    memset(filepath, 0, sz);
    if (fsh->prefix != NULL) {
        strcpy(filepath, fsh->prefix);
        offset = strlen(fsh->prefix);
    }
    sprintf(filepath + offset, "%u", (unsigned int)finfo);
}

file_id fsh_makeid(fshandler *fsh, file_n num) {
    return (fsh->bid << 16) | num;
}

file_n fsh_idtonum(file_id id) { return (file_n)id; }

void fsh_init(fshandler *fsh, int bid, const char *prefix) {
    szqfid_init(&fsh->avail);
    szqfid_push(&fsh->avail, 0);

    (fsh->tfiles) = 0;
    (fsh->cfiles) = 0;
    (fsh->bid) = bid;
    fst_init(&(fsh->exist));
    for (int i = 0; i < MAX_FILES_FS; ++i) {
        (fsh->open)[i] = NULL;
    }
    fsh->prefix = NULL;
    if (prefix != NULL) {
        fsh->prefix = (char *)calloc((strlen(prefix) + 2), sizeof(char));
        strcpy(fsh->prefix, prefix);
    }
}

bool fsh_has(file_id *finfo, fshandler *fsh, const char *name) {
    return fst_has(&(fsh->exist), name, finfo);
}

int fsh_rewindfp_name(fshandler *fsh, const char *name) {
    file_id *finfo;
    fstrie *exist = &(fsh->exist);
    FILE **open = fsh->open;

    if (!fst_has(exist, name, finfo)) {
        pferr("REWIND: file doesn't exist");
        return -1;
    }

    file_n num = fsh_idtonum(*finfo);
    if (open[num] == NULL) {
        pferr("REWIND: file isnt open");
        return -2;
    }

    fseek(open[num], 0, SEEK_SET);
    return 0;
}

int fsh_rewindfp(fshandler *fsh, file_id finfo) {
    file_n num = fsh_idtonum(finfo);
    FILE **open = fsh->open;

    if (open[num] == NULL) {
        pferr("REWIND: file isnt open");
        return -1;
    }

    fseek(open[num], 0, SEEK_SET);
    return 0;
}

int fsh_filesz(file_id *finfo, fshandler *fsh, const char *name) {
    fstrie *exist = &(fsh->exist);
    FILE **open = fsh->open;
    if (!fst_has(exist, name, finfo)) return -1;
    file_n num = fsh_idtonum(*finfo);

    if (open[num] == NULL) {
        char filepath[500];
        genfname(filepath, fsh, 500, *finfo);

        open[num] = fopen(filepath, "rb+");
        if (open[num] == NULL) {
            pferr("READ: cant open file %s in bucket %i\n", filepath, fsh->bid);
            return -1;
        }
    }

    fseek(open[num], 0, SEEK_END);
    int retval = ftell(open[num]);
    rewind(open[num]);

    return retval;
}

int fsh_read(file_id *finfo, void *dest, fshandler *fsh, const char *name,
             int sz) {
    fstrie *exist = &(fsh->exist);
    FILE **open = fsh->open;

    if (!fst_has(exist, name, finfo)) return -1;
    file_n num = fsh_idtonum(*finfo);

    if (open[num] == NULL) {
        char filepath[500];
        genfname(filepath, fsh, 500, *finfo);

        open[num] = fopen(filepath, "rb+");
        if (open[num] == NULL) {
            pferr("READ: cant open file %s in bucket %i\n", filepath, fsh->bid);
            return -1;
        }
    }

    int res = fread(dest, sizeof(char), sz, open[num]);
    if (ferror(open[num])) {
        pferr("READ: error reading from file %s\n", name);
        clearerr(open[num]);
        return -1;
    }
    return res;
}

int fsh_newfile(file_id *finfo, fshandler *fsh, const char *name) {
    fstrie *exist = &(fsh->exist);
    FILE **open = fsh->open;
    szqfid *avail = &(fsh->avail);

    // if file didnt exist, gotta create it, give it a file_id,
    // and add it to the exist file
    if (fst_has(exist, name, finfo)) return 1;  // 1 if it already existed

    if (fsh->cfiles >= MAX_FILES_FS) {
        pferr("NEWFILE: couldnt create %s, bucket %i full\n", name, fsh->bid);
        return -1;
    }

    file_n num = *(szqfid_peek(avail));  // retrieve next available number
    *finfo = fsh_makeid(fsh, num);       // generate finfo
    char fpath[500];

    genfname(fpath, fsh, 500, *finfo);
    printf("%s %s\n", name, fpath);
    FILE *temp = fopen(fpath, "wb+");  // create the file

    if (temp == NULL) {
        pferr("NEWFILE: couldn't create file %s\n", fpath);
        return -1;
    }

    // make the permanent changes to the struct
    szqfid_pop(avail);

    // if the next available number was bigger than all previous ones,
    // add 1 to total files and push it to the queue. file nums
    // are 0 indexed. it's last in case something above fails
    if (num >= (fsh->tfiles) && (fsh->tfiles) + 1 < MAX_FILES_FS) {
        // push a new ind only if it's less than MAX_FILES
        ++(fsh->tfiles);
        szqfid_push(avail, fsh->tfiles);
    }

    // printf("next num %i\n", num);
    fst_add(exist, name, *finfo);  // add to exist
    open[num] = temp;
    ++(fsh->cfiles);  // there's a new file open now.

    return 0;  // it didnt exist before and i created it succesfully
}

int fsh_write(file_id *finfo, fshandler *fsh, const char *name, const void *src,
              int sz, bool replace) {
    FILE **open = fsh->open;

    // if file didnt exist, gotta create it, give it a file_id,
    // and add it to the exist file
    // 1 if file exsted before, 0 otherwise
    int ex_res = fsh_newfile(finfo, fsh, name);
    if (ex_res == -1) {
        pferr("WRITE: file %s can't be created in bucket %i\n", name, fsh->bid);
        return -1;
    }

    file_n num = fsh_idtonum(*finfo);  // shorten it to find the file number
    // printf("%i %i %i\n", finfo, (finfo >> 16), num);

    if (open[num] == NULL) {  // file existed but isnt open
        char filepath[500];
        genfname(filepath, fsh, 500, *finfo);

        if (replace) {
            open[num] = fopen(filepath, "wb+");
        } else {
            open[num] = fopen(filepath, "rb+");
            fseek(open[num], 0, SEEK_END);  // THIS COULD CAUSE AN ERROR MAYBE
        }

        if (open[num] == NULL) {
            pferr("WRITE: file %s exists in bucket %i, but cant open it\n",
                  filepath, fsh->bid);
            return -1;
        }
    } else if (ex_res && replace) {  // if it was already open
        char filepath[500];
        genfname(filepath, fsh, 500, *finfo);
        int res = fclose(open[num]);
        if (res != 0) {
            pferr("WRITE: cant close file %s in bucket %i\n", filepath,
                  fsh->bid);
            return -1;
        }

        open[num] = fopen(filepath, "wb+");
        if (open[num] == NULL) {
            pferr("WRITE: cant re-open file %s in bucket%i\n", filepath,
                  fsh->bid);
            return -1;
        }
    }

    // printf("%s %s %i num %i\n", name, src, sz, num);

    size_t written = fwrite(src, 1, sz, open[num]);
    if (ferror(open[num])) {
        pferr("WRITE: error printing to file %s\n", name);
        clearerr(open[num]);
        return -1;
    }

    return written;
}

int fsh_close(file_id *finfo, fshandler *fsh, const char *name) {
    fstrie *exist = &(fsh->exist);
    FILE **open = fsh->open;

    if (!fst_has(exist, name, finfo)) return -1;

    file_n num = fsh_idtonum(*finfo);

    // if its open, close it and reset open[num]
    if (open[num] != NULL) {
        int res = fclose(open[num]);
        if (res != 0) {
            pferr("CLOSE: cant close file %s in bucket %i\n", name, fsh->bid);
            return -1;
        }
        open[num] = NULL;
    }

    return 0;
}

int fsh_remove(file_id *finfo, fshandler *fsh, const char *name) {
    fstrie *exist = &(fsh->exist);
    FILE **open = fsh->open;
    szqfid *avail = &(fsh->avail);

    int res = fsh_close(finfo, fsh, name);
    if (res == -1) {
        pferr("REMOVE: %s in bucket %i doesnt exist or cant be closed\n", name,
              fsh->bid);
        return -1;
    }

    char filepath[500];
    genfname(filepath, fsh, 500, *finfo);
    /*printf("%s %s\n", name, filepath);*/
    // ackchually remove it
    res = remove(filepath);
    if (res == -1) {
        pferr("REMOVE: %s in bucket %i couldnt be remove()'d\n", filepath,
              fsh->bid);
        return -1;
    }

    --(fsh->cfiles);
    fst_del(exist, name, finfo);
    szqfid_push(avail, fsh_idtonum(*finfo));  // push the filename

    return 0;
}

int clear_fshand(void **args) {
    char *buf = (char *)args[0];
    int offset = *((int *)args[1]);
    trie_node *curr = (trie_node *)args[2];
    fshandler *fshand = (fshandler *)args[3];

    buf[offset] = '\0';

    FILE **open = fshand->open;
    file_n num = fsh_idtonum(curr->data);

    if (open[num] != NULL) {
        int res = fclose(open[num]);
        if (res != 0) {
            pferr("DESTROY: cant close file %s in bucket %i\n", buf,
                  fshand->bid);
            return -1;
        }
        open[num] = NULL;
    }

    char fpath[500];
    genfname(fpath, fshand, 500, curr->data);

    int res = remove(fpath);
    if (res == -1) {
        pferr("DESTROY: %s in bucket %i couldnt be remove()'d\n", fpath,
              fshand->bid);
        return -1;
    }

    return 0;
}

void fsh_destroy(fshandler *fsh) {
    void *args[1] = {fsh};
    fst_walk(&(fsh->exist), clear_fshand, args, 1);

    fst_destroy(&(fsh->exist));
    if (fsh->prefix != NULL) free(fsh->prefix);
}

// end of file handler---------------------
