#ifndef BUFFER_H
#define BUFFER_H
#include <stdint.h>

#define BUFFER_SIZE 1170 // (73 registers/s) * 16s 

struct log_record {
    int64_t time_us;
    double u[2];
    double y[3];
    char process_name;
};

typedef struct{
    int in;
    int out;
    struct log_record buf[BUFFER_SIZE]; 
} Buffer;

extern void buffer_init(Buffer *b);
extern int buffer_count(const Buffer *b);
extern int buffer_append(struct log_record item, Buffer *b);
extern int buffer_take(struct log_record *item, Buffer *b);

#endif