#include "reqzo.h"

void reqzo_ser(void* buf, reqzo* req) {
    uint8_t* cp = (uint8_t*)buf;
    cp[0] = req->type;
    cp[1] = ((req->bucket) >> 8) & 0xFF;
    cp[2] = (req->bucket) & 0xFF;
    cp[3] = ((req->info_len) >> (3 * 8)) & 0xFF;
    cp[4] = ((req->info_len) >> (2 * 8)) & 0xFF;
    cp[5] = ((req->info_len) >> 8) & 0xFF;
    cp[6] = (req->info_len) & 0xFF;
}

void reqzo_deser(void* buf, reqzo* req) {
    uint8_t* cp = (uint8_t*)buf;
    req->type = (uint8_t)cp[0];
    req->bucket = (cp[1] << 8) | (cp[2]);
    req->info_len = (cp[3] << 24) | (cp[4] << 16) | (cp[5] << 8) | (cp[6]);
}
