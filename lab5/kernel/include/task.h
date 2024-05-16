#ifndef __TASK_H
#define __TASK_H

#include "task_heap.h"

void task_heap_init();
void do_task();

extern task_heap* task_hp;
extern int cur_priority;

#endif