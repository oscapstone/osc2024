#ifndef _SCHED_H
#define _SCHED_H

#include "type.h"
#include "list.h"
#include "signal.h"
#include "task.h"


task_ptr    current_task();

void  		scheduler_init();
void        scheduler_add(task_ptr tsk);
void        scheduler_add_shell(task_ptr tsk);
void        scheduler_kill(task_ptr tsk, int32_t status);
task_ptr    scheduler_find(int32_t pid);

void		schedule();
void 		kill_zombies();

void        schedule_tail();
void 		scheduler_tick(byteptr_t);

int32_t     get_pid();


#endif