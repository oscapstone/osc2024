#ifndef TIMER_H
#define TIMER_H

#include "bool.h"

extern void enable_core0_timer(void);
extern void disable_core0_timer(void);
extern unsigned long get_current_time(void);

void set_core_timer_timeout(unsigned int seconds);


typedef void (*timer_callback)(char* msg);

void add_timer(timer_callback cb,
               char* msg,
               unsigned int duration,
               bool periodic);

void core_timer_handle_irq(void);

void set_timeout(char* msg, unsigned int duration, bool periodic);


#endif /* TIMER_H */
