#include "task.h"
#include "alloc.h"

task_p_queue* t_priority_queue;

void init_task_queue(void) {
    t_priority_queue = (task_p_queue *)simple_malloc(sizeof(task_p_queue));

    for (int i = 0; i < MIN_PRIORITY; i++) {
        t_priority_queue->priority_q[i]->front = NULL;
        t_priority_queue->priority_q[i]->end = NULL;
    }
}

void enqueue_task(task* t, int priority) {
    // The priority queue is empty initially.
    if (t_priority_queue->priority_q[priority]->front == NULL) {
        t_priority_queue->priority_q[priority]->front = t;
        t_priority_queue->priority_q[priority]->end = t;
    } else {
        t_priority_queue->priority_q[priority]->end->next = t;
        t_priority_queue->priority_q[priority]->end = t_priority_queue->priority_q[priority]->end->next;
    }
}