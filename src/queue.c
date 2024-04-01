#include "queue.h"

struct queue read_buffer = {{{0}}, 0, 0, 64};
struct queue write_buffer = {{{0}}, 0, 0, 64};

struct queue task_queue = {{{0}}, 0, 0, 10};

void enqueue_char(struct queue *q, char c)
{
    q->buffer[q->head] = c;
    q->head = (q->head + 1) % q->size;
}

char dequeue_char(struct queue *q)
{
    char c = q->buffer[q->tail];
    q->tail = (q->tail + 1) % q->size;
    return c;
}

void enqueue_softirq(struct queue *q, void (*action)(void *data), void *data)
{
    q->softirq_vec[q->head].action = action;
    // q->softirq_vec[q->head].data = data;
    q->head = (q->head + 1) % q->size;
}

struct softirq_action dequeue_softirq(struct queue *q)
{
    struct softirq_action action = q->softirq_vec[q->tail];
    q->tail = (q->tail + 1) % q->size;
    return action;
}

int is_empty(struct queue *q)
{
    return q->head == q->tail;
}