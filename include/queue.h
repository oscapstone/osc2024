#ifndef __QUEUE_H__
#define __QUEUE_H__

struct queue {
    char buffer[64];
    int head;
    int tail;
    int size;
};

extern struct queue read_buffer, write_buffer;

void enqueue(struct queue *q, char c);
char dequeue(struct queue *q);
int is_empty(struct queue *q);



#endif // __QUEUE_H__