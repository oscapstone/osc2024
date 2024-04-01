/**
 * Read/Write buffer queue
 * Softirq queue
*/
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "interrupt.h"


struct queue {
    union {
        char buffer[64];
        struct softirq_action softirq_vec[6];
    };
    int head;
    int tail;
    int size;
};

extern struct queue read_buffer, write_buffer;

void enqueue_char(struct queue *q, char c);
char dequeue_char(struct queue *q);
void enqueue_softirq(struct queue *q, void (*action)(void *data), void *data);
struct softirq_action dequeue_softirq(struct queue *q);
int is_empty(struct queue *q);



#endif // __QUEUE_H__