#ifndef _TIMER_H
#define _TIMER_H

#include "util.h"
#include "type.h"
#include "io.h"
#include "exception.h"
#include "mem.h"
#include "thread.h"

typedef struct thread thread_t;

typedef struct {
    void (*func_arg)(char*);
    void (*func)();
    int expire_time;
    char* arg;
    int with_arg;
    int valid;
} timer_t;



void init_time_queue();
void set_kernel_timer();
// void set_sched_timer();

void enable_core_timer();
void disable_core_timer();

uint64_t get_core_freq();
uint64_t get_core_count();

void add_timer(void (*callback)(), int expire_time);
void add_timer_arg(void (*callback)(char*), char* arg, int expire_time);
void set_timeout(char*, uint32_t);

void print_current_time();
void sleep(void (*wakeup_func)(), uint32_t duration);

void one_sec_pass();
void core_timer_handler();
void two_sec_timer_handler();

uint32_t timer_empty();

#endif