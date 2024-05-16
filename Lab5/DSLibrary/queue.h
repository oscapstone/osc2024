#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stddef.h>
#include "doubly_list.h"

typedef struct queue {
    doubly_linked_list_t list;  // Using the doubly linked list to store queue elements
} Queue;

void    init_queue(Queue* q);
void    enqueue(Queue *queue, void *data);
void*   dequeue(Queue *queue);
int     queue_element_count(Queue* queue);

#endif