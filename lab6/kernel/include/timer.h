#ifndef TIMER_H
#define TIMER_H

#include "list.h"

//https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p13
#define CORE0_TIMER_IRQ_CTRL PHYS_TO_VIRT(0x40000040)
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
void cpu_timer_enable();
void set_core_timer_interrupt(unsigned long long expire_time);
void set_core_timer_interrupt_by_tick(unsigned long long tick);
void set_core_timer_interrupt_first();
void add_Node_timer(list_head_t *head, timer_event_t *entry);
void add_timer(timer_callback_t callback,void* arg, unsigned long long expire_time, int bytick);
void pop_timer() ;
int timer_list_size();

unsigned long long get_clock_tick();
unsigned long long get_clock_freq();
unsigned long long get_clock_time();

void two_second_alert(const char *str);
void core_timer_interrupt_disable_alternative();
void core_timer_handler();
void timer_event_callback(timer_event_t *timer_event);
void timer_list_init();
unsigned long long get_tick_plus_s(unsigned long long second);
void print_timer();

#endif