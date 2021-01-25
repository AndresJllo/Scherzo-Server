#include "scherzo-util.h"


int get_line(const char* prompt, char* buf, size_t sz) {
    char ch;
    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }

    if (fgets(buf, sz, stdin) == NULL) return -1;
    int buflen = strlen(buf);

    if (buf[buflen - 1] != '\n') {
        while (((ch = getchar()) != '\n') && ch != EOF) 0;
    }

    buf[buflen - 1] = '\0';
    return buflen;
}

int sendall(int socket, char* buf, int len, int* sent) {
    if (len == 0) return 0;
    *sent = 0;
    int left = len, n = 0;

    do {
        n = send(socket, buf + *sent, left, 0);
        (*sent) += n, left -= n;
        if (n <= 0) pferr("SENDALL: there was an error sending data\n");

    } while (*sent < len && n > 0);

    return 0 - (n == -1);
}
