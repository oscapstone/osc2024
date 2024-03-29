#include "queue.h"

struct queue read_buffer = {{0}, 0, 0, 64};
struct queue write_buffer = {{0}, 0, 0, 64};

void enqueue(struct queue *q, char c)
{
    q->buffer[q->head] = c;
    q->head = (q->head + 1) % q->size;
}

char dequeue(struct queue *q)
{
    char c = q->buffer[q->tail];
    q->tail = (q->tail + 1) % q->size;
    return c;
}

int is_empty(struct queue *q)
{
    return q->head == q->tail;
}