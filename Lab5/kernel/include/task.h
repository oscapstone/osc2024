#ifndef _TASK_H
#define _TASK_H

#include "mem.h"
#include "io.h"
#include "thread.h"

#define LOW_PRIO 0x100
#define TIMER_PRIO 0x010
#define UART_PRIO 0x001

typedef struct task_struct task_t;
typedef struct thread thread_t;

typedef void (*task_callback_t)(void);

struct task_struct {
    task_t* prev;
    task_t* next;
    int prio;
    task_callback_t callback;
};

void init_task_queue();
void add_task(task_callback_t callback, int prio);
void pop_task();

#endif