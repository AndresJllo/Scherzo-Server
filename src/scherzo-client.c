#ifndef _GNU_SOURCE  // ifnotdef so that it works with gcc and g++
#define _GNU_SOURCE  // turns out this is implicitly defined in g++
#endif               // you need this for getaddrinfo and addrinfo

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#include "reqzo.h"
#include "scherzo-util.h"

#define MAXBACKLOG 200
#define RECVBUFSIZE 5001
int sockfd = -1;
extern int errno;
struct {
    FILE* fp;
    char fname[251];
    int size;
} fileinfo;

int writefifo;


int parse_input(void* inbuf, size_t sz, reqzo* t, int fifofd) {
    int fifo_read = read(fifofd, inbuf, sz);
    if (fifo_read <= 0) return fifo_read;
            
    uint8_t* cp = (uint8_t*)inbuf;
    t->type = cp[0] - '0';
        

    if (t->type != LISTB) {
        t->bucket = (cp[1] << 8) | cp[2];
        printf("recieved %i %i\n", t->type, t->bucket);
    }

    switch (t->type) {
        case SEND:
        case RECV:
        case DELF: {
            memset(fileinfo.fname, 0, sizeof fileinfo.fname);
            strcpy(fileinfo.fname, (char*)(inbuf) + 3);

            printf("filename %s\n", fileinfo.fname);

            if (t->type == RECV) fileinfo.fp = fopen(fileinfo.fname, "rb");
            if (t->type == SEND) fileinfo.fp = fopen(fileinfo.fname, "wb+");

            if (t->type != DELF && fileinfo.fp == NULL) {
                pferr("error opening file\n");
                return -1;
            }

            t->info_len = strlen(fileinfo.fname);

            if (t->type != DELF) {
                fseek(fileinfo.fp, 0, SEEK_END);
                fileinfo.size = ftell(fileinfo.fp);
                rewind(fileinfo.fp);
                t->info_len += 1 + fileinfo.size;  // null or $
                printf("lengths %li + %i = %i\n", strlen(fileinfo.fname) + 1,
                       fileinfo.size, t->info_len);
            }

        } break;
        case NEWB:
        case DELB:
        case LIST:
        case LISTB:
            break;

        default:
            return -1;
    }

    reqzo_ser(inbuf, t);
    return sizeof(reqzo);
}

int del_file(char* buf, reqzo* last) {
    memset(buf, 0, sizeof(reqzo) + 1);  // clear out the OK

    int sent_bytes, res;
    res = sendall(sockfd, fileinfo.fname, last->info_len, &sent_bytes);
    if (res == -1) {
        pferr("SENDFILE: error sending filename\n");
        return -1;
    }

    int recv_bytes = recv(sockfd, buf, sizeof(reqzo) + 1, 0);
    printf("%i\n", recv_bytes);

    reqzo t;
    reqzo_deser(buf, &t);
    memset(buf, 0, sizeof(reqzo) + 1);  // clear out the OK/ERR

    return (t.type == OK) ? 0 : -1;
}

int send_file(char* buf, reqzo* last) {
    memset(buf, 0, sizeof(reqzo) + 1);  // clear out the OK

    int res, sent_last = 0, read_last = 0;
    int offset = strlen(fileinfo.fname);
    strcpy(buf, fileinfo.fname);
    buf[offset++] = '$';

    read_last = fread(buf + offset, 1, 4096, fileinfo.fp);
    if (ferror(fileinfo.fp)) {
        pferr("SENDFILE: error reading file\n");
        return -1;
    }

    res = sendall(sockfd, buf, read_last + offset, &sent_last);
    if (res == -1) {
        pferr("SENDFILE: error sending filename\n");
        return -1;
    }
    printf("read %i sent %i\n", read_last, sent_last);

    memset(buf, 0, RECVBUFSIZE);
    while ((read_last = fread(buf, 1, 4096, fileinfo.fp)) > 0) {
        int sent_last;
        if (sendall(sockfd, buf, read_last, &sent_last) == -1) {
            pferr("SENDFILE: error sending file\n");
            perror("oy vey");
            return -1;
        }
        memset(buf, 0, RECVBUFSIZE);
    }

    puts("done sending");
    if (ferror(fileinfo.fp)) {
        pferr("SENDFILE: error reading file\n");
        perror("oy vey");
        return -1;
    }

    fclose(fileinfo.fp);
    fileinfo.fp = NULL;
    fileinfo.size = 0;

    return 0;
}

int recv_file(char* buf, reqzo* last) {
    memset(buf, 0, sizeof(reqzo) + 1);  // clear out the OK

    int sent_bytes;
    int res = sendall(sockfd, fileinfo.fname, last->info_len, &sent_bytes);
    if (res == -1) {
        pferr("SENDFILE: error sending filename\n");
        return -1;
    }

    int recv_bytes = recv(sockfd, buf, 4096, 0);
    printf("%i\n", recv_bytes);

    reqzo t;
    reqzo_deser(buf, &t);
    if (t.type != OK) return -1;

    recv_bytes -= sizeof(reqzo);

    int written_bytes = fwrite(buf + sizeof(reqzo), 1, recv_bytes, fileinfo.fp);

    memset(buf, 0, RECVBUFSIZE);
    while (recv_bytes < t.info_len) {
        res = recv(sockfd, buf, 4096, 0);
        if (res < 0) {
            pferr("SENDFILE: error %i recieving\n", res);
            perror("oy vey");
            return -1;
        }
        fwrite(buf, 1, res, fileinfo.fp);
        recv_bytes += res;
        memset(buf, 0, RECVBUFSIZE);
    }

    printf("got %i expected %i\n", recv_bytes, t.info_len);
    puts("done recieving file");

    fclose(fileinfo.fp);
    fileinfo.fp = NULL;
    fileinfo.size = 0;

    return 0;
}

int recv_flist(char* buf, reqzo* t, int recv_bytes) {
    char fname[202];
    memset(fname, 0, sizeof fname);
    int index = 0;
    writefifo = open("gui_pipe", O_WRONLY);

    for (int i = sizeof(reqzo); i < recv_bytes; ++i) {
        fname[index] = buf[i];
        if (buf[i] == '$') {
            printf("in first %s\n", fname);
            write(writefifo, fname, index);

            memset(fname, 0, sizeof fname);
            index = 0;
        }
        ++index;
    }

    int res;
    recv_bytes -= sizeof(reqzo);  // ignore the OK
    memset(buf, 0, RECVBUFSIZE);
    printf("need %i got %i\n", t->info_len, recv_bytes);
    while (recv_bytes < t->info_len) {
        index = 0;
        res = recv(sockfd, buf, 4096, 0);
        if (res < 0) {
            pferr("SENDFILE: error %i recieving\n", res);
            return -1;
        }

        for (int i = 0; i < res; ++i) {
            fname[index] = buf[i];
            if (buf[i] == '$') {
                printf("in second %s\n", fname);
                write(writefifo, fname, index);

                memset(fname, 0, sizeof fname);
                index = 0;
            }
            ++index;
        }
        printf("got %i this time\n", res);
        recv_bytes += res;
        memset(buf, 0, RECVBUFSIZE);
    }
    puts("finished list");
    close(writefifo);
    return 0;
}

int recv_blist(char* buf, reqzo* t, int recv_bytes) {
    writefifo = open("gui_pipe", O_WRONLY);

    char numbuf[200];
    memset(numbuf, 0, sizeof numbuf);

    for (int i = sizeof(reqzo); i < recv_bytes; i += 2) {
        uint16_t num = (buf[i] << 8) | buf[i + 1];
        sprintf(numbuf, "%u", num);
        numbuf[strlen(numbuf)] = '$';
        /*printf("length %i\n", strlen(numbuf));*/
        write(writefifo, numbuf, strlen(numbuf));

        memset(numbuf, 0, sizeof numbuf);
    }

    int res;
    recv_bytes -= sizeof(reqzo);  // ignore the OK

    memset(buf, 0, RECVBUFSIZE);
    while (recv_bytes < t->info_len) {
        res = recv(sockfd, buf, 4096, 0);
        if (res < 0) {
            pferr("SENDFILE: error %i recieving\n", res);
            perror("oy vey");
            return -1;
        }

        for (int i = 0; i < res; i += 2) {
            uint16_t num = (buf[i] << 8) | buf[i + 1];
            sprintf(numbuf, "%u", num);
            numbuf[strlen(numbuf)] = '$';
            /*printf("length %i\n", strlen(numbuf));*/
            write(writefifo, numbuf, strlen(numbuf));
            memset(numbuf, 0, sizeof numbuf);
        }
        recv_bytes += res;
        memset(buf, 0, RECVBUFSIZE);
    }
    close(writefifo);
    return 0;
}

int handle_req(char* buf, reqzo* last, int recv_bytes) {
    reqzo t;
    reqzo_deser(buf, &t);
    printf("handling: %i %i %i\n", t.type, t.bucket, t.info_len);
    printf("recieved: %i\n", recv_bytes);

    if (t.type == ERR) {
        pferr("server replied with error\n");
        return -1;
    }
    int retval = -1;
    switch (last->type) {
        case DELB:
        case NEWB:
            puts("hello");
            retval = 0;
            break;
        case DELF:
            retval = del_file(buf, last);
            break;
        case SEND:
            retval = recv_file(buf, last);
            break;
        case RECV:
            retval = send_file(buf, last);
            break;
        case LIST:
            retval = recv_flist(buf, &t, recv_bytes);
            break;
        case LISTB:
            retval = recv_blist(buf, &t, recv_bytes);
            break;
    }

    if (retval == -1) pferr("server replied with error\n");

    return retval;
}

int main() {
    // START: intialize networking stuffs---------------------------------------
    struct addrinfo hints;
    struct addrinfo* res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int res1 = getaddrinfo("localhost", "8888", &hints, &res);
    if (res1 != 0) {
        pferr("couldn't get host info it seems\n");
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    int status = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (status != 0) {
        pferr("couldnt connect to host fool\n");
        return -1;
    }
    // END: initialize networking stuffs----------------------------------------

    int fifofd = open("client_pipe", O_RDONLY);
    char user_in[1024], buf[RECVBUFSIZE];
    reqzo t;

    while (1) {
        memset(user_in, 0, sizeof user_in);
        memset(buf, 0, sizeof buf);

        int input_read = parse_input(user_in, 250, &t, fifofd);

        if (input_read <= 0) {
            pferr("couldn't read input\n");
            close(fifofd);
            return 0;
        } else {
            int bytes_sent;
            if (sendall(sockfd, user_in, input_read, &bytes_sent) == -1) {
                pferr("error sending req, %i bytes sent\n", bytes_sent);
            }
        }
        int recv_bytes = recv(sockfd, buf, 4096, 0);
        if (recv_bytes == -1) pferr("error while recieving response\n");
        if (recv_bytes == 0) {
            puts("server closed connection");
            return 0;
        }
        handle_req(buf, &t, recv_bytes);
    }

    freeaddrinfo(res);  // res is apparently
}
