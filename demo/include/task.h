#ifndef TASK_H
#define TASK_H

#include "../include/my_stdint.h"
#include "../include/my_stddef.h"
#include "../include/my_stdlib.h"
#include "../include/exception.h"
#include "../include/timer.h"
#include "../include/my_string.h"
#include "../include/uart.h"

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
task_t* create_task(void (*callback)(void *),int priority);
int add_task_to_queue(task_t *newTask);
void test_tesk1(void);
void test_tesk2(void);
void test_tesk3(void);
void ExecTasks(void);

#endif