#include "fshandler.h"

#include <stdio.h>

// fshandler-------------------------------
// TODO: make this constructor populate exist and
// open accordingly

int fshandler::rewindfp(const char *name) {
    file_id *finfo;
    if (exist.has_str(name, finfo) == -1) {
        pferr("REWIND: file doesn't exist");
        return -1;
    }
    
    file_n num = idtonum(*finfo);
    if(open[num] == nullptr) {
        pferr("REWIND: file isnt open");
        return -2;
    }

    fseek(open[num], 0, SEEK_SET);
    return 0;
}

int fshandler::readfs(file_id *finfo, const char *name, void *dest, int sz) {
    if (!exist.has_str(name, finfo)) return -1;
    file_n num = idtonum(*finfo);
    printf("num %i\n", num);

    if (open[num] == nullptr) {
        open[num] = fopen(name, "rb+");
        if (open[num] == nullptr) {
            pferr("READ: cant open file %s in bucket %i\n", name, bid);
            return -1;
        }
    }

    int res = fread(dest, 1, sz, open[num]);
    if (ferror(open[num])) {
        pferr("READ: error reading from file %s\n", name);
        clearerr(open[num]);
        return -1;
    }
    return res;
}

int fshandler::newfilefs(file_id *finfo, const char *name) {
    // if file didnt exist, gotta create it, give it a file_id,
    // and add it to the exist file
    if (exist.has_str(name, finfo)) return 1;  // 1 if it already existed

    if (curr_files >= MAX_FILES) {
        pferr("NEWFILE: couldnt create %s, bucket %i full\n", name, bid);
        return -1;
    }

    file_n num = *avail.peek();       // retrieve next available number
    FILE *temp = fopen(name, "wb+");  // create the file

    if (temp == nullptr) {
        pferr("NEWFILE: couldn't create file %s\n", name);
        return -1;
    }

    // make the permanent changes to the struct
    avail.pop();

    // if the next available number was bigger than all previous ones,
    // add 1 to total files and push it to the queue. file nums
    // are 0 indexed. it's last in case something above fails
    if (num >= total_files) {
        // push a new ind only if it's less than MAX_FILES
        if (total_files + 1 < MAX_FILES) {
            avail.push(++total_files);
        }
        open[num] = temp;
    }

    // printf("next num %i\n", num);
    *finfo = makeid(num);         // generate finfo
    exist.add_str(name, *finfo);  // add to exist
    open[num] = temp;
    ++curr_files;  // there's a new file now.

    return 0;  // it didnt exist before and i created it succesfully
}

int fshandler::writefs(file_id *finfo, const char *name, const void *src,
                       int sz, bool replace) {
    // if file didnt exist, gotta create it, give it a file_id,
    // and add it to the exist file
    int ex_res =
        newfilefs(finfo, name);  // 1 if file exsted before, 0 otherwise
    if (ex_res == -1) {
        pferr("WRITE: file %s can't be created in bucket %i\n", name, bid);
        return -1;
    }

    file_n num = idtonum(*finfo);  // shorten it to find the file number

    // printf("%i %i %i\n", finfo, (finfo >> 16), num);

    if (open[num] == nullptr) {  // file existed but isnt open
        open[num] = (replace) ? fopen(name, "wb+") : fopen(name, "rb+");
        if (open[num] == nullptr) {
            pferr("WRITE: file %s exists in bucket %i, but cant open it\n",
                  name, bid);
            return -1;
        }
    } else if (ex_res && replace) {  // if it was already open
        int res = fclose(open[num]);
        if (res != 0) {
            pferr("WRITE: cant close file %s in bucket %i\n", name, bid);
            return -1;
        }

        open[num] = fopen(name, "wb+");
        if (open[num] == nullptr) {
            pferr("WRITE: cant re-open file %s in bucket%i\n", name, bid);
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

int fshandler::closefs(file_id *finfo, const char *name) {
    if (!exist.has_str(name, finfo)) return -1;

    file_n num = idtonum(*finfo);

    // if its open, close it and reset open[num]
    if (open[num] != nullptr) {
        int res = fclose(open[num]);
        if (res != 0) {
            pferr("CLOSE: cant close file %s in bucket %i\n", name, bid);
            return -1;
        }
        open[num] = nullptr;
    }

    return 0;
}

int fshandler::removefs(file_id *finfo, const char *name) {
    int res = closefs(finfo, name);
    if (res == -1) {
        pferr("REMOVE: %s in bucket %i doesnt exist or cant be closed\n", name,
              bid);
        return -1;
    }

    // printf("%s\n", name);
    // ackchually remove it
    res = remove(name);
    if (res == -1) {
        pferr("REMOVE: %s in bucket %i couldnt be remove()'d\n", name, bid);
        return -1;
    }

    --curr_files;
    exist.del_str(name);
    avail.push(idtonum(*finfo));  // push the filename

    return 0;
}

bool fshandler::has_file(file_id *finfo, const char *name) {
    return exist.has_str(name, finfo);
}

// end of file handler---------------------
