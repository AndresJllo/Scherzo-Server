#ifndef SCHERZOUTIL
#define SCHERZOUTIL
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

#define pferr(...) fprintf(stderr, __VA_ARGS__)

int get_line(const char* prompt, char* buf, size_t sz);

int sendall(int socket, char* buf, int len, int* sent);

#endif
