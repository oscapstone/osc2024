#ifndef TASK_H
#define TASK_H

#include "kernel/uart.h"
#include "kernel/allocator.h"
#include "kernel/timer.h"

// Structure to represent a task
typedef struct task{
    struct task* prev;
    struct task* next;
    void (*callback)(void *);
    void *data;
    int priority;
}task_t;

extern task_t* task_head;
extern task_t* task_tail;

// Function to insert a task into the task queue based on priority
int task_create_DF1(void (*callback)(void *), void* data, int priority);
// create task that takes no argument
int task_create_DF0(void (*callback)(), int priority);
void task_callback(void);
void ExecTasks(void);

#endif