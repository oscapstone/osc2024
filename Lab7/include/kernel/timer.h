#ifndef TIMER_H
#define TIMER_H

#include "kernel/INT.h"
#include "kernel/allocator.h"
#include "kernel/uart.h"

// timer queue made from double linked list
typedef struct task_timer{
    struct task_timer *prev;
    struct task_timer *next;
    void (*callback)(void *);
    void *data;
    unsigned long long deadline;
}task_timer_t;

extern task_timer_t* timer_head;
extern task_timer_t* timer_tail;

int add_timer(void (*callback)(void *), void* data, int after);
int add_timer_NA(void (*callback)(void), int after);
void print_callback(void *str);
void settimeout(char *str, int second);

#endif