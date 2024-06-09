#ifndef TIMER_H
#define TIMER_H

#define TIMER_IRQ_PRIORITY 1

#include "bool.h"

#define SCHED_CYCLE (get_freq() >> 5)
#define S(n)        ((n) * get_freq())
#define MS(n)       (S(n) / 1000)
#define GET_S(n)    ((n) / get_freq())
#define GET_MS(n)   ((n) % get_freq())


extern void enable_core0_timer(void);
extern void disable_core0_timer(void);
extern unsigned long get_current_time(void);
extern unsigned long get_current_ticks(void);
extern unsigned long get_freq(void);
extern void set_core_timer_timeout_ticks(unsigned long ticks);
extern void set_core_timer_timeout_secs(unsigned long seconds);

typedef void (*timer_callback)(char* msg);

int timer_init(void);

void print_msg(char* msg);

void add_timer(timer_callback cb,
               char* msg,
               unsigned long duration,
               bool is_periodic);

void core_timer_handle_irq(void);

#endif /* TIMER_H */
