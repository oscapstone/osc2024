#ifndef __TASK_HEAP_H
#define __TASK_HEAP_H

typedef void (*task_callback)();

typedef struct task{
    task_callback callback; // the function to call
    int priority;           // the priority of task
} task;

typedef struct task_heap{
    task *arr;
    int size;
    int capacity;
} task_heap;

task_heap *create_task_heap(int capacity);
void task_heap_insertHelper(task_heap *h, int index);
void task_heap_heapify(task_heap *h, int index);
task task_heap_extractMin(task_heap *h);
void task_heap_insert(task_heap *h, task t);

#endif