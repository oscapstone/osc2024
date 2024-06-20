#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "type.h"
#include "util.h"
#include "thread.h"
#include "shell.h"
#include "mini_uart.h"
#include "timer.h"
#include "mem.h"

typedef struct thread thread_t;
typedef struct trap_frame trap_frame_t;

void init_scheduler();

void idle();
void schedule();
void kill_zombies();

void set_ctx(uint64_t);

void thread_create(void (*func)(void));
void thread_exec(void*, uint32_t);
void thread_exit();
int copy_thread(trap_frame_t*);
void new_user_thread(void*);
void user_thread_run();

void push_rq(thread_t*);
thread_t* pop_rq();

void check_RQ();

// For scheduler test
void foo();

#endif