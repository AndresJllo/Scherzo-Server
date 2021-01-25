#include <stdio.h>
#include <stdlib.h> /* srand, rand */
#include <string.h>
#include <sys/stat.h>
#include <time.h> /* time */
#include <unistd.h>

#include "../src/fshandler.h"
#include "../src/fstrie.h"
#include "../src/szdcue.h"

#define pferr(...) fprintf(stderr, __VA_ARGS__)

char all[67] =
    "-./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

void rnd_str(char* buf, int len) {
    for (int i = 0; i < len; ++i) {
        int sel = rand() % 66;  // last chat is nullterminator
        sel -= (sel == 2);      // if its the /, spare me the troble
        buf[i] = all[sel];
    }
    buf[len] = '\0';
}

int char_num(char a) {
    if (a >= '-' && a <= '9') return a - '-';
    if (a >= 'A' && a <= 'Z') return a - 'A' + 13;
    if (a == '_') return ('Z' - 'A' + 13) + 1;
    if (a >= 'a' && a <= 'z') return a - 'a' + 40;
    pferr("the char %c is not recognized\n", a);
    return -1;
}

void testchars() {
    for (int i = 0; i < 66; ++i) {
        if (char_num(all[i]) != i) {
            printf("what %i %c\n", i, all[i]);
            return;
        } else {
            printf("ok %i ", i);
        }
    }
    puts("");
}

void file_test() {
    FILE* fp = fopen("test.txt", "wb");

    fwrite("haha", sizeof(char), 4, fp);
    fclose(fp);
    fwrite("hehe", sizeof(char), 4, fp);
}

void trie_shit() {
    int iters = 100000000;
    while (iters--) {
        long long i = -(int)1e9;

        printf("%lli\n", i);
        fstrie t;
        fst_init(&t);

        fst_add(&t, "h", 68);
        fst_add(&t, "haha", 69);
        fst_add(&t, "hahaha", 70);

        uint32_t val = -9;
        printf("%ui\n", fst_del(&t, "hahaha", NULL));
        printf("%ui ", fst_has(&t, "haha", &val));
        printf("%i\n", val);
        printf("%ui ", fst_has(&t, "h", &val));
        printf("%i\n", val);
        printf("%ui ", fst_has(&t, "hahaha", &val));
        printf("%i\n", val);

        puts("hello");
        fst_destroy(&t);
    }
}

void file_handler_write() {
    fshandler hand;
    fsh_init(&hand, 4, "tests/scherzo-fs/");

    fstrie k;
    char filename[100000], msg[500000], tmp[500000];
    const int iter = 100;
    file_id files[iter];
    memset(files, -1, sizeof files);
    for (int i = 0; i < iter; ++i) {
        memset(filename, 0, sizeof filename);  // ensure null terminator
        memset(tmp, 0, sizeof tmp);
        memset(msg, 0, sizeof msg);

        sprintf(filename, "fuck%i", i);

        for (int j = 0; j <= i; ++j) {
            sprintf(tmp, "fuck%i", j);
            strcat(msg, tmp);
        }
        // printf("ready %i\n", i);
        // printf("%s\n", filename);
        // i can use strlen cuz i know msg has 0s at the end

        int res = -1;
        res = fsh_write(&files[i], &hand, filename, msg, strlen(msg), 0);
        if (res == -1) {
            puts("good bye");
        }
    }

    puts("-----");
    int start = 0, end = 50;

    for (int j = start; j < end; ++j) {
        memset(filename, 0, sizeof filename);  // ensure null terminator
        memset(msg, 0, sizeof msg);

        sprintf(filename, "fuck%i", j);
        sprintf(msg, "hahaha%i", j);

        file_id haha = -1;
        int tmp = -1;
        tmp = fsh_write(&haha, &hand, filename, msg, strlen(msg), 0);

        if (tmp == -1 || haha != files[j]) {
            pferr("FUCK WHAT THE FILE DOESNT MATCH\n");
        }
    }

    fsh_destroy(&hand);
}

void file_handler_remove() {
    fshandler k;
    fsh_init(&k, 2, "tests/scherzo-fs/");

    char filename[50000], msg[50000];
    const char f1[] = "fuckfuck";
    const char f2[] = "fuhkofuck";
    const char f3[] = "fuckfuch";

    memset(filename, 0, sizeof filename);
    memset(msg, 0, sizeof msg);
    strcat(msg, "shitdicks");
    file_id f1info;
    fsh_write(&f1info, &k, f1, msg, strlen(msg), 0);
    printf("file1 info %i %i %i\n", f1info, f1info >> 16, (file_n)f1info);

    strcat(msg, "butforrealyo");
    file_id f2info;
    fsh_write(&f2info, &k, f2, msg, strlen(msg), 0);
    printf("file2 info %i %i %i\n", f2info, f2info >> 16, (file_n)f2info);

    strcpy(msg, "nah nvm family lmao");
    file_id f3info;
    fsh_write(&f3info, &k, f3, msg, strlen(msg), 0);
    printf("file3 info %i %i %i\n", f3info, f3info >> 16, (file_n)f3info);

    file_id finfo = 666;

    printf("active files %i\n", k.cfiles);

    file_id delf1;
    fsh_remove(&delf1, &k, f1);
    printf("removed f1 %s\n", f1);
    bool res = fsh_has(&finfo, &k, f1);
    printf("res f1 %i finfo %i\n", res, finfo);

    res = fsh_has(&finfo, &k, f3);
    printf("res f3 %i finfo %i\n", res, finfo);
    printf("active files %i\n", k.cfiles);

    res = fsh_has(&finfo, &k, f2);
    printf("res f2 %i finfo %i\n", res, finfo);
    printf("active files %i\n", k.cfiles);

    const char f4[] = "shitshit";
    const char f5[] = "shotshot";

    strcat(msg, "fuck you cunts");
    file_id f4info;
    fsh_write(&f4info, &k, f4, msg, strlen(msg), 0);
    printf("file4 info %i %i %i\n", f4info, f4info >> 16, (file_n)f4info);

    finfo = 666;
    res = fsh_has(&finfo, &k, f4);
    printf("res %i finfo %i\n", res, finfo);

    strcat(msg, "fr tho");
    file_id f5info;
    fsh_write(&f5info, &k, f5, msg, strlen(msg), 0);
    printf("file5 info %i %i %i\n", f5info, f5info >> 16, (file_n)f5info);

    finfo = 666;
    res = fsh_has(&finfo, &k, f4);
    printf("res %i finfo %i\n", res, finfo);

    printf("active files %i\n", k.cfiles);
    fsh_remove(&finfo, &k, f4);
    printf("removed %s\n", f4);

    finfo = 666;
    res = fsh_has(&finfo, &k, f4);
    printf("res %i finfo %i\n", res, finfo);
    finfo = 666;
    res = fsh_has(&finfo, &k, f5);
    printf("res %i finfo %i\n", res, finfo);
    printf("active files %i\n", k.cfiles);

    fsh_destroy(&k);
}

void file_handler_remove2() {
    char filename[5000], msg[1000000];
    char file_db[1000][100];
    file_id files[MAX_FILES_FS];
    bool deleted[MAX_FILES_FS];
    printf("%i\n", MAX_FILES_FS);
    memset(deleted, 0, sizeof deleted);
    fshandler k;
    fsh_init(&k, 1, "tests/scherzo-fs/");
    int str_len = 60, iters = 600;

    FILE* fps[MAX_FILES_FS];
    int total_files = 0;
    for (int i = 0; i < iters; ++i) {
        memset(filename, 0, sizeof filename);
        memset(msg, 0, sizeof msg);

        rnd_str(filename, str_len);

        int head = 0;
        for (int j = 0; j <= i; ++j) {
            head += sprintf(msg + head, "fuck%i", j);
        }

        int res = fsh_write(&files[i], &k, filename, msg, strlen(msg), 0);

        strcpy(&file_db[i][0], filename);
        if (res == -1) {
            pferr("blimey at file_handler_remove2, 1st loop\n");
        } else {
            ++total_files;
        }
    }

    int start = 10, end = 30;
    bool chosen[total_files];
    memset(chosen, false, sizeof chosen);
    for (int t = start; t < end; ++t) {
        int j;
        do {
            j = rand() % total_files;
        } while (chosen[j]);
        chosen[j] = true;

        file_id finfo;
        int res = fsh_remove(&finfo, &k, file_db[j]);
        if (res == -1 || finfo != files[j]) {
            pferr("blimey at file_handler_remove2, 2nd loop\n");
        } else {
            deleted[j] = true;
        }

        for (int i = 0; i < iters; ++i) {
            file_id shit;
            bool haha = fsh_has(&shit, &k, file_db[i]);
            if (!deleted[i] && !haha) {
                pferr("blimey 2 at file_handler_remove2, 2nd loop\n");
            }
            if (deleted[i] && haha) {
                pferr("blimey 3 at file_handler_remove2, 2nd loop\n");
            }
        }
    }
    printf("total files %i\n", k.cfiles);
    puts("FINISHED");
    fsh_destroy(&k);
}

void file_handler_read() {
    char msg[10];
    char filename[] = "hello.txt";
    char outname[] = "bye.txt";
    fshandler k;
    fsh_init(&k, 4, "tests/scherzo-fs/");

    int iters = 2;
    file_id finfo;
    for (int i = 0; i < iters; ++i) {
        memset(msg, all[i % 66], sizeof msg);
        fsh_write(&finfo, &k, filename, msg, sizeof msg, 0);
    }

    char buf[1001];
    int read = 0, last = 0;
    memset(buf, 0, sizeof buf);

    fsh_rewindfp(&k, finfo);

    while (last = fsh_read(&finfo, buf, &k, filename, 100)) {
        read += last;
        printf("read so far: %i last %i\n", read, last);
        fsh_write(&finfo, &k, outname, buf, last, 0);
        memset(buf, 0, sizeof buf);
    }
    fsh_destroy(&k);
}

void file_handler_read2() {
    char filename[] = "tests/testfile1";
    char outname[] = "longeyescpy.png";
    char outname2[] = "longeyescpy2.png";

    fshandler k;
    fsh_init(&k, 5, "tests/scherzo-fs/");
    file_id finfo;

    char buf[1001];
    int read = 0, last = 0;
    memset(buf, 0, sizeof buf);

    FILE* fp = fopen(filename, "r+");
    while (last = fread(buf, sizeof(char), 10, fp)) {
        read += last;
        printf("read so far: %i last %i\n", read, last);
        printf("buf %s\n", buf);
        fsh_write(&finfo, &k, outname, buf, last, 0);
        memset(buf, 0, sizeof buf);
    }

    read = 0, last = 0;
    memset(buf, 0, sizeof buf);

    fseek(fp, 0, SEEK_SET);  // reset reading pointer
    while (last = fread(buf, sizeof(char), 10, fp)) {
        read += last;
        printf("read so far: %i last %i\n", read, last);
        printf("buf %s\n", buf);
        fsh_write(&finfo, &k, outname2, buf, last, 0);
        memset(buf, 0, sizeof buf);
    }
    fsh_destroy(&k);
}

void file_handler_prefix() {
    fshandler k;
    fsh_init(&k, 4, "tests/scherzo-fs/");

    char buf[] = "hahaha";

    file_id finfo;

    fsh_write(&finfo, &k, "hello.txt", buf, strlen(buf), 1);
    fsh_destroy(&k);
}

int main() {
    unsigned int a = (2 << 16) | 1;
    unsigned short b = (unsigned short)a;          // last bits
    unsigned short c = (unsigned short)(a >> 16);  // first bits
    unsigned int d = printf("a %i, c %i, b %i\n", a, c, b);

    srand(time(NULL));
    // testchars();
    // file_test();
    // trie_shit();
    /*file_handler_write();*/
    /*file_handler_remove();*/
    /*file_handler_remove2();*/
    /*file_handler_read();*/
    /*file_handler_read2();*/
    /*file_handler_prefix();*/
}
