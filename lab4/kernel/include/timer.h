#ifndef TIMER_H
#define TIMER_H

#include "list.h"
#include "uart.h"

#define CORE0_TIMER_IRQ_CTRL ((0x40000040))
#define STR(x) #x
#define XSTR(x) STR(x)

typedef void (*timer_callback_t)(char *);

typedef struct timer_event
{
    list_head_t listhead;
    unsigned long long expire_time; 
    timer_callback_t callback; 
    char *args; 
} timer_event_t;



void core_timer_enable();
void core_timer_disable();
void core_timer_interrupt_enable();
void core_timer_interrupt_disable();
void set_core_timer_interrupt(unsigned long long ticks);
void add_Node_timer(list_head_t *head, timer_event_t *entry);
void add_timer(timer_callback_t callback,void* arg, unsigned long long expire_time);
void pop_timer() ;

unsigned long long get_clock_tick();
unsigned long long get_clock_freq();
unsigned long long get_clock_time();

void two_second_alert(const char *str);
void core_timer_interrupt_disable_alternative();
void core_timer_handler();


void print_timer();

#endif