#ifndef _TIMER_H_
#define _TIMER_H_

#include "list.h"

#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define STR(x) #x
#define XSTR(x) STR(x)

typedef void (*timer_callback_t)(char *);

typedef struct {
    unsigned long long expired_time;
    timer_callback_t callback;
    char *args;
    struct list_head list;
} timer_node;

void core_timer_enable();
void core_timer_disable();
void core_timer_interrupt_enable();
void core_timer_interrupt_disable();
void set_core_timer_interrupt();
void set_core_timer_interrupt_permanent();

void add_timer(timer_callback_t callback, void *arg, unsigned long long expired_time);
void add_node(struct list_head *head, timer_node *entry);
void pop_timer();
void core_timer_handler();

#endif