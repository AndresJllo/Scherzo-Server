/* TODO: make this read a config file
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fshandler.h"
#include "fstrie.h"
#include "kidpool.h"
#include "reqzo.h"
#include "scherzo-util.h"
#include "szdcue.h"

// these 2 are different defines cuz theyre
// template arguments, and i want MAXCLIENTS
// to be editable with a file.
#define CONNCAP 200       // max connections
#define MAXERR 200        // max queued errors
#define MAXCUED 200       // max queued connections
#define MAXBUCKS 200      // max buckets
#define MAXCUEDBUCKS 200  // max queued bucket req

typedef struct {
    int fi, se;
} ii;

KIDPOOL(MAXBUCKS, MAXCUEDBUCKS)
SZQ(int, MAXERR, err)
SZQ(ii, MAXCUED, con)

char root[500];

// error queue, cuz i use it in every thread
pthread_mutex_t err_mutex;
sem_t err_spaces, err_items;
szqerr errs;
//------------------------------------

// START: USEFUL FUNCTIONS TO SAVE SPACE---------------------------------
void add_fatal(int err_client) {
    sem_wait(&err_spaces);
    pthread_mutex_lock(&err_mutex);

    szqerr_push(&errs, err_client);

    pthread_mutex_unlock(&err_mutex);
    sem_post(&err_items);
}

int send_msg(int client, int m) {
    char buf[20];
    memset(buf, 0, 20);

    reqzo t = {(uint8_t)m, MAXBUCKS + 1, MAXBUCKS + 1};
    reqzo_ser(buf, &t);

    // send MSG
    int res, sent_bytes;
    res = sendall(client, buf, sizeof(reqzo), &sent_bytes);
    if (res < 0) {
        pferr("SENDOK: error sending MSG to client %i res %i\n", client, res);
        return -1;
    }

    return sent_bytes;
}
// END: USEFUL FUNCTIONS TO SAVE SPACE-----------------------------------

// FUNCTION FOR THE ERROR SENDER THREAD----------------------------------
void *send_errs(void *raw) {
    reqzo tgt;
    char buf[100];

    while (1) {
        sem_wait(&err_items);
        pthread_mutex_lock(&err_mutex);
        int temp = *(szqerr_peek(&errs));
        szqerr_pop(&errs);
        pthread_mutex_unlock(&err_mutex);
        sem_post(&err_spaces);

        memset(buf, 0, sizeof buf);
        tgt.type = (uint8_t)ERR;
        tgt.info_len = temp;
        reqzo_ser(buf, &tgt);

        int bytes_sent;
        sendall(temp, buf, sizeof(reqzo), &bytes_sent);

        close(temp);
    }

    pthread_exit(NULL);
}
// END: FUNCTION FOR THE ERROR SENDER THREAD-----------------------------

// START: FUNCTIONS FOR BUCKET HANDLER THREADS---------------------------

int del_file(bcomm *c, fshandler *fshand, int *ch_count) {
    char fname[202];
    memset(fname, 0, sizeof fname);

    int sent_ok = send_msg(c->client, OK);
    if (sent_ok == -1) {
        pferr("RECVFILE: couldnt send ok to client %i\n", c->client);
        return -1;
    }

    // recv name
    int res, recv_bytes = 0;
    while (recv_bytes < c->info_len) {
        res = recv(c->client, fname + recv_bytes, 200, 0);
        if (res < 0) {
            pferr("SENDFILE: error %i recieving client %i\n", res, c->client);
            return -1;
        }
        recv_bytes += res;
    }

    file_id finfo;
    res = fsh_remove(&finfo, fshand, fname);

    if (res != -1) {  // if it had it
        *ch_count -= strlen(fname) + 1;
        printf("ch_count -%li\n", strlen(fname) + 1);
        send_msg(c->client, OK);
    } else {
        send_msg(c->client, ERR);
    }

    return 0;
}

int send_file(bcomm *c, fshandler *fshand) {
    char fname[202];
    memset(fname, 0, 201);
    printf("expect %i\n", c->info_len);

    if (send_msg(c->client, OK) == -1) {
        pferr("SENDFILE: couldnt send ok to client %i\n", c->client);
        return -1;
    }

    // recv name
    int res, recv_bytes = 0;
    while (recv_bytes < c->info_len) {
        res = recv(c->client, fname + recv_bytes, 200, 0);
        if (res < 0) {
            pferr("SENDFILE: error %i recieving client %i\n", res, c->client);
            return -1;
        }
        recv_bytes += res;
    }

    printf("SEND: filename %s\n", fname);
    file_id finfo;
    int fsize = fsh_filesz(&finfo, fshand, fname);
    if (fsize == -1) {
        send_msg(c->client, ERR);
        return 0;
    }
    printf("SEND: fsize %i\n", fsize);

    char buf[5000];
    reqzo t = {OK, 0, fsize};

    memset(buf, 0, sizeof buf);
    reqzo_ser(buf, &t);

    int read_bytes = fsh_read(&finfo, buf + sizeof(reqzo), fshand, fname, 4096);

    int sent_bytes;
    res = sendall(c->client, buf, read_bytes + sizeof(reqzo), &sent_bytes);
    printf("%i\n", sent_bytes);
    if (res < 0) {
        pferr("SENDFILE: error %i sending to client %i\n", res, c->client);
        return -1;
    }

    memset(buf, 0, sizeof buf);
    while ((read_bytes = fsh_read(&finfo, buf, fshand, fname, 4096)) > 0) {
        res = sendall(c->client, buf, read_bytes, &sent_bytes);
        if (res < 0) {
            pferr("SENDFILE: error %i sending to client %i\n", res, c->client);
            return -1;
        }
        memset(buf, 0, sizeof buf);
    }

    fsh_close(&finfo, fshand, fname);
    puts("hello there");
    return 0;
}

int recv_file(bcomm *c, fshandler *fshand, int *ch_count) {
    char fname[202], buf[5001];
    memset(buf, 0, 4091);
    memset(fname, 0, 201);
    printf("expect %i\n", c->info_len);

    int sent_ok = send_msg(c->client, OK);
    if (sent_ok == -1) {
        pferr("RECVFILE: couldnt send ok to client %i\n", c->client);
        return -1;
    }

    // recv name
    int recv_bytes = recv(c->client, buf, 5000, 0);
    int fname_end = 0;
    while (buf[fname_end] != '$') {
        ++fname_end;
        if (fname_end >= recv_bytes) {
            fname_end = -1;
            break;
        }
    }

    if (fname_end == -1) {
        pferr("RECVFILE:no filename terminator found");
        return -1;
    }

    int offset = fname_end + 1;
    while (fname_end > 0) {
        --fname_end;
        fname[fname_end] = buf[fname_end];
    }

    file_id finfo;
    if (!fsh_has(&finfo, fshand, fname)) {
        *ch_count += offset;  // increase total char count
        printf("ch_count +%i\n", offset);
    }

    fsh_write(&finfo, fshand, fname, buf + offset, recv_bytes - offset, 1);

    int res;
    memset(buf, 0, sizeof(buf));
    while (recv_bytes < c->info_len) {
        res = recv(c->client, buf, 5000, 0);
        if (res < 0) {
            pferr("RECVFILE: error %i recieving client %i\n", res, c->client);
            return -1;
        }

        fsh_write(&finfo, fshand, fname, buf, res, 0);
        memset(buf, 0, sizeof(buf));
        recv_bytes += res;
    }

    printf("got %i expected %i\n", recv_bytes, c->info_len);
    fsh_close(&finfo, fshand, fname);
    return 0;
}

int send_names(void **args) {
    char *buf = (char *)args[0];
    int offset = *((int *)args[1]);
    int client = *((int *)args[3]);

    buf[offset] = '$';
    printf("%s\n", buf);
    int res, sent_bytes;
    res = sendall(client, buf, offset + 1, &sent_bytes);
    if (res < 0) {
        pferr("WALKTRIE: error %i sending to client %i\n", res, client);
    }
    printf("sent %i\n", sent_bytes);
    buf[offset] = '\0';

    return (res < 0) ? -1 : 0;
}

int list_files(bcomm *c, fshandler *fshand, int ch_count) {
    char buf[202];
    memset(buf, 0, sizeof buf);
    reqzo t = {OK, 0, ch_count};
    reqzo_ser(buf, &t);

    int res, sent_bytes;
    res = sendall(c->client, buf, sizeof(reqzo), &sent_bytes);
    if (res < 0) {
        pferr("LISTBUCKET: error %i sending to client %i\n", res, c->client);
        return -1;
    }

    void *args[1] = {&(c->client)};

    return fst_walk(&(fshand->exist), send_names, args, 1);
}

void *hand_buck(void *raw) {
    void **args = (void **)raw;
    sem_t *buck_spaces = (sem_t *)args[0];
    sem_t *buck_items = (sem_t *)args[1];
    szqkidp *buck_cue = (szqkidp *)args[2];
    int buck = *((int *)args[3]);

    sem_t *cued_spaces = (sem_t *)args[4];
    sem_t *cued_items = (sem_t *)args[5];
    pthread_mutex_t *cued_mutex = (pthread_mutex_t *)args[6];
    szqcon *cued = (szqcon *)args[7];
    kidpool *pool = (kidpool *)args[8];

    free(args[3]), free(args);

    int ch_count = 0;

    fshandler fshand;
    fsh_init(&fshand, buck, root);
    int res = -1;

    while (1) {
        sem_wait(buck_items);

        bcomm c = *(szqkidp_peek(buck_cue));
        szqkidp_pop(buck_cue);

        sem_post(buck_spaces);
        switch (c.type) {
            case DELF:
                res = del_file(&c, &fshand, &ch_count);
                break;
            case SEND:
                res = send_file(&c, &fshand);
                break;
            case RECV:
                res = recv_file(&c, &fshand, &ch_count);
                break;
            case LIST:
                res = list_files(&c, &fshand, ch_count);
                break;
            case DELB: {
                fsh_destroy(&fshand);
                pthread_mutex_lock(&(pool->openb_mutex));
                pool->openbucks[buck] = false;
                --(pool->bucks_count);
                pthread_mutex_unlock(&(pool->openb_mutex));

                send_msg(c.client, OK);

                int dead_c[CONNCAP];  // TODO: use something other than this
                memset(dead_c, false, sizeof(dead_c));
                while (sem_trywait(buck_items) == 0) {
                    bcomm d = *(szqkidp_peek(buck_cue));
                    szqkidp_pop(buck_cue);
                    dead_c[d.client] = true;

                    sem_post(buck_spaces);  // see notes for 2020-10-25
                }

                sem_destroy(buck_items);
                sem_destroy(buck_spaces);

                for (int i = 0; i < CONNCAP; ++i) {
                    if (dead_c[i] && sem_trywait(cued_spaces) == 0) {
                        pthread_mutex_lock(cued_mutex);

                        ii ret_client = {i, -1};
                        szqcon_push(cued, ret_client);

                        pthread_mutex_unlock(cued_mutex);
                        sem_post(cued_items);
                    }
                }

                // stop ignoring the client that requested this
                pthread_mutex_lock(cued_mutex);
                ii ret_client = {c.client, 0};
                szqcon_push(cued, ret_client);
                pthread_mutex_unlock(cued_mutex);
                sem_post(cued_items);

                puts("done destroying the handler");
                pthread_exit(NULL);
            } break;
        }

        if (sem_trywait(cued_spaces) == 0) {
            pthread_mutex_lock(cued_mutex);

            ii ret_client = {c.client, res};
            szqcon_push(cued, ret_client);

            printf("pushed %i with msg %i\n", c.client, res);
            pthread_mutex_unlock(cued_mutex);
            sem_post(cued_items);
        }
    }
}
// END: FUNCTIONS FOR BUCKET HANDLER THREADS-----------------------------

// START: FUNCTIONS FOR THE CONNECTION POLLER THREAD--------------------

int read_client(int clientfd, kidpool *pool, sem_t *cued_spaces,
                sem_t *cued_items, pthread_mutex_t *cued_mutex, szqcon *cued) {
    char buf[20];
    memset(buf, 0, 20);
    int recv_bytes = recv(clientfd, buf, 20, 0);
    printf("READCLIENT: read %i\n", recv_bytes);
    if (recv_bytes == 0) return -2;

    reqzo t;
    reqzo_deser(buf, &t);

    printf("recieved: %i %i %i from %i\n", t.type, t.bucket, t.info_len,
           clientfd);

    if (t.bucket >= pool->max_buckets) return -2;

    pthread_mutex_lock(&(pool->openb_mutex));
    bool new_b = !(pool->openbucks)[t.bucket];
    pthread_mutex_unlock(&(pool->openb_mutex));

    if (new_b && (t.type != NEWB && t.type != LISTB)) return -2;

    sem_t *buck_spaces = &((pool->buck_spaces)[t.bucket]);
    sem_t *buck_items = &((pool->buck_items)[t.bucket]);
    szqkidp *buck_cue = &((pool->buck_cue)[t.bucket]);

    int retval = 0;

    switch (t.type) {
        case DELB:
        case SEND:
        case RECV:
        case DELF:
        case LIST: {
            bcomm c = {t.type, t.info_len, clientfd};
            sem_wait(buck_spaces);
            szqkidp_push(buck_cue, c);
            sem_post(buck_items);

        } break;
        case NEWB: {
            if (!new_b || t.bucket >= MAXBUCKS) {
                send_msg(clientfd, ERR);
                retval = -1;
            } else {
                /*printf("%i\n", pool->max_cued);*/

                sem_init(buck_spaces, 0, pool->max_cued);
                sem_init(buck_items, 0, 0);
                szqkidp_init(buck_cue);
                uint16_t *buck = (uint16_t *)malloc(sizeof(uint16_t));
                *buck = t.bucket;

                void **args = (void **)malloc(9 * sizeof(void *));
                args[0] = buck_spaces;
                args[1] = buck_items;
                args[2] = buck_cue;
                args[3] = buck;
                args[4] = cued_spaces;
                args[5] = cued_items;
                args[6] = cued_mutex;
                args[7] = cued;
                args[8] = pool;

                pthread_t *new_thread = &((pool->buck_hands)[t.bucket]);

                pthread_mutex_lock(&(pool->openb_mutex));
                pool->openbucks[t.bucket] = true;
                ++(pool->bucks_count);
                pthread_mutex_unlock(&(pool->openb_mutex));

                // tell the client his bucket was created
                send_msg(clientfd, OK);
                pthread_create(new_thread, NULL, hand_buck, args);
                return -1;
            }
        } break;
        case LISTB: {
            // 2 bytes per bucknum, since its a short anyway
            pthread_mutex_lock(&(pool->openb_mutex));
            int list_sz = 2 * pool->bucks_count;
            pthread_mutex_unlock(&(pool->openb_mutex));
            printf("buckets %i\n", list_sz);

            reqzo ok_sz = {OK, 0, list_sz};

            int bsz = list_sz + sizeof(reqzo) + 1;
            char buf[bsz];
            memset(buf, 0, bsz);
            reqzo_ser(buf, &ok_sz);

            int head = sizeof(reqzo);
            for (int i = 0; i < pool->max_buckets; ++i) {
                if (pool->openbucks[i]) {
                    uint16_t buck = (uint16_t)i;
                    buf[head] = (buck >> 8) & 0xFF;
                    buf[head + 1] = buck & 0xFF;
                    head += 2;
                }
            }

            int sent_bytes;
            if (sendall(clientfd, buf, bsz - 1, &sent_bytes) < 0) {
                pferr("LISTB: error sending MSG to client %i\n", clientfd);
                return -2;
            }

            return -1;
        } break;
        default:
            retval = -2;
    }

    return retval;
}

void *poll_conns(void *raw) {
    // you could prolly spawn a number of these tbh
    void **args = (void **)raw;
    sem_t *cued_spaces = (sem_t *)args[0];
    sem_t *cued_items = (sem_t *)args[1];
    pthread_mutex_t *cued_mutex = (pthread_mutex_t *)args[2];
    szqcon *cued = (szqcon *)args[3];
    free(args);

    kidpool pool;
    bool ignore[CONNCAP];  // TODO: use something other than this

    kidpool_init(&pool);
    memset(ignore, false, sizeof(ignore));
    int cconns = 0, cthreads = 0, timeout = 100;
    struct epoll_event ev, events[CONNCAP];
    int epfd = epoll_create(CONNCAP);

    while (1) {
        int err_client = -1;

        if (sem_trywait(cued_items) == 0) {
            pthread_mutex_lock(cued_mutex);
            ii client = *(szqcon_peek(cued));
            szqcon_pop(cued);

            if (client.se == 1) {  // new connection
                (cconns >= CONNCAP) ? (err_client = client.fi) : ({
                    ev.events = EPOLLIN | EPOLLHUP | EPOLLPRI | EPOLLERR;
                    ev.data.fd = client.fi;

                    int add_res =
                        epoll_ctl(epfd, EPOLL_CTL_ADD, client.fi, &ev);
                    (add_res == 0) ? (++cconns) : (err_client = client.fi);
                });
            } else {  // returned
                ignore[client.fi] = false;
                if (client.se == -1) {  // with errors
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client.fi, NULL);
                    err_client = client.fi;
                }
            }

            pthread_mutex_unlock(cued_mutex);
            sem_post(cued_spaces);
        }

        if (err_client != -1) {
            pferr("error adding or returned by client %i\n", err_client);
            add_fatal(err_client);
        }

        if (cconns == 0) continue;

        int ndfs = epoll_wait(epfd, events, CONNCAP, timeout);
        if (ndfs < 0) pferr("error waiting in epoll\n");

        for (int i = 0; i < ndfs; ++i) {
            int client = events[i].data.fd;
            if (!ignore[client]) {
                ignore[client] = true;
                int read_res = read_client(client, &pool, cued_spaces,
                                           cued_items, cued_mutex, cued);

                if (read_res < 0) {  // didnt add to anything
                    // TODO: differentiate between fatal and not fatal
                    ignore[client] = false;
                    if (read_res < -1) {
                        pferr("malformed request\n");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, client, NULL);
                        add_fatal(client);
                    }
                }
            }
        }
    }

    pthread_exit(NULL);
}

// END: FUNCTIONS FOR THE CONNECTION POLLER THREAD----------------------

volatile int quit = 0;

void handle_sigint(int sig_num) { quit = 1; }

int main(int argc, char **argv) {
    // maximum connections to be listened for
    unsigned int MAXCLIENTS = 30;
    signal(SIGINT, handle_sigint);
    memset(root, 0, 500);

    if (argc > 1) {
        strcpy(root, argv[1]);
        printf("%s\n", root);
    }

    // START: initialize error queue and thread--------------------------------
    pthread_t err_sender;

    pthread_mutex_init(&err_mutex, NULL);
    sem_init(&err_spaces, 0, MAXERR);
    sem_init(&err_items, 0, 0);
    szqerr_init(&errs);
    // END: initialize error queue and thread----------------------------------

    // START: initialize connections containers--------------------------------
    sem_t cued_spaces, cued_items;
    pthread_mutex_t cued_mutex;
    szqcon cued;
    void **args1 = (void **)malloc(4 * sizeof(void *));
    pthread_t conn_poller;

    sem_init(&cued_spaces, 0, MAXCUED);
    sem_init(&cued_items, 0, 0);
    pthread_mutex_init(&cued_mutex, NULL);
    szqcon_init(&cued);
    args1[0] = &cued_spaces, args1[1] = &cued_items, args1[2] = &cued_mutex;
    args1[3] = &cued;
    // END: initialize conntections containers---------------------------------

    // START: initialize listening socket, bind it-----------------------------
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /*fcntl(sockfd, F_SETFL, O_NONBLOCK);*/

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        pferr("couldnt bind sockfd %i\n", sockfd);
        return -1;
    }

    if (listen(sockfd, MAXCLIENTS)) {
        pferr("couldnt listen on socket %i\n", sockfd);
        return -1;
    }
    // END: initialize listening socket, bind it--------------------------------

    // START THREADS
    pthread_create(&err_sender, NULL, send_errs, NULL);
    pthread_create(&conn_poller, NULL, poll_conns, args1);
    // -------------------------------------------------------------------------

    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    socklen_t clientsz = sizeof(client);
    puts("STARTING UP...");

    while (quit == 0) {
        int clientfd = accept(sockfd, (struct sockaddr *)&client, &clientsz);
        if (clientfd != -1) {
            puts("added new client");
            sem_wait(&cued_spaces);
            pthread_mutex_lock(&cued_mutex);

            ii new_client = {clientfd, 1};
            szqcon_push(&cued, new_client);

            pthread_mutex_unlock(&cued_mutex);
            sem_post(&cued_items);
        }
    }

    puts("cya later");
    close(sockfd);
    pthread_cancel(err_sender);
    pthread_cancel(conn_poller);
    sem_destroy(&cued_spaces);
    sem_destroy(&cued_items);
    pthread_mutex_destroy(&err_mutex);
    sem_destroy(&err_spaces);
    sem_destroy(&err_items);
}
