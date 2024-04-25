#ifndef TIMER_H
#define TIMER_H
#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define STR(x) #x //it converts the macro argument x into a string literal after macro replacement
#define XSTR(x) STR(x)

#include "list.h"


typedef struct timer {
    // struct list_head listhead;
    struct timer *prev;
    struct timer *next;
    unsigned long long interrupt_time;  //store as tick time after cpu start
    void* callback; // interrupt -> timer_callback -> callback(args)
    char* args; // need to free the string by event callback function
} timer;

void core_timer_interrupt_enable();
void core_timer_interrupt_disable();
void set_core_timer_interrupt(unsigned long long sec);
void core_timer_interrupt_disable_alternative();

void timer_list_init();
void add_timer(void* callback, unsigned long long timeout, char* args);
void poptimer();
void core_timer_handler();

unsigned long long get_tick_plus_s(unsigned long long second);

#endif