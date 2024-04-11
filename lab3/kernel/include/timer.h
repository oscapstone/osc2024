#ifndef __TIMER_H__
#define __TIMER_H__

#include "types.h"

struct event {
    struct event *next;
    void *args;
    uint32_t expired_time;
    void (*callback)(void*);
};

void infinite();
void core_timer_enable();
void core_timer_disable();
void add_core_timer(void (*func)(void*), void *args, uint32_t time);
void set_core_timer(uint32_t time);
void remove_core_timer();
void print_boot_time();
void print_core_timer_message(void *args);
uint64_t get_core_frequency();
uint64_t get_core_count();

#endif