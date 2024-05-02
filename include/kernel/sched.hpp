#pragma once

#include "mm/mm.hpp"
#include "thread.hpp"

#define STACK_SIZE PAGE_SIZE

extern "C" {
// sched.S
void switch_to(Kthread* prev, Kthread* next);
}

void push_rq(Kthread* thread);
Kthread* pop_rq();

void kill_zombies();
void schedule_init();
void schedule();
