#ifndef __TIMER_H__
#define __TIMER_H__

#define CORE0_TIMER_IRQ_CTRL 0x40000040

#include "type.h"

struct time_t{
    uint64_t timeout;
    void (*callback_func)(void*);
    char arg[64];
    struct time_t *next;
};

extern void core_timer_enable();
extern void core_timer_disable();
extern unsigned long long get_cpu_cycles();
extern unsigned int get_cpu_freq();
extern void set_timer_asm(unsigned int sec);

void print_time_handler(char* arg);
void timer_update_handler();

void set_timer(unsigned int sec);
void add_timer(void(*callback_func)(void*), void* args, uint32_t sec);
void print_timeout_msg(void* msg);

void time_head_init();

#endif