#include "buffer.h"

void buffer_init(Buffer *b){
    b->in = 0;
    b->out = 0;
}
// return the free slots the buffer has
int buffer_count(const Buffer *b){
    int count;
    count = (b->in) - (b->out);
    
    if((b->in) < (b->out)){
        count += BUFFER_SIZE;
    }
    return count;
}

int buffer_append(struct log_record item, Buffer *b){
    if(buffer_count(b) >= (BUFFER_SIZE - 1)) return -1;
    b->buf[b->in] = item;
    b->in = ((b->in) + 1) % BUFFER_SIZE;
    return 0;
}

int buffer_take(struct log_record *item, Buffer *b){
    if(b->in == b->out) return -1;
    *item = b->buf[b->out];
    b->out = ((b->out) + 1) % BUFFER_SIZE;
    return 0;
}