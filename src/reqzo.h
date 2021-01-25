#ifndef REQZO
#define REQZO
/* the form of a request
 * that might be answered by a server
 * thread
 */
#include <stdint.h>
enum req_type { OK, ERR, DELF, SEND, RECV, LIST, DELB, NEWB, LISTB };

typedef struct _reqzo {
    uint8_t type;
    uint16_t bucket;
    uint32_t info_len;
} reqzo;

void reqzo_ser(void* buf, reqzo* req);
void reqzo_deser(void* buf, reqzo* req);

#endif
