#ifndef TIMER_H
#define TIMER_H

#include "../include/my_stdint.h"
#include "../include/my_stddef.h"
#include "../include/my_stdlib.h"
#include "../include/exception.h"
#include "../include/my_string.h"
#include "../include/uart.h"

#define CORE0_TIMER_IRQ_CTRL    (volatile unsigned int*)(0x40000040)

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

task_timer_t *create_timer(void (*callback)(void *),void* message, int seconds);
int timer_add_queue(task_timer_t *temp);
void timer_callback(void *str);
void setTimeout (char *str, int second);

#endif