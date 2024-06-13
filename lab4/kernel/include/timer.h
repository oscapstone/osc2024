#ifndef	_TIMER_H_
#define	_TIMER_H_

#define CORE0_TIMER_IRQ_CTRL 0x40000040
#include "list.h"

// Def for timer event
typedef struct timer_event {
    struct list_head listhead;
    unsigned long long interrupt_time;  //store as tick time after cpu start
    void *callback;                     // interrupt -> timer_callback -> callback(args)
    char* args;                         // arguments, need to be free by callback
} timer_event_t;

void                core_timer_enable();
void                core_timer_disable();
void                set_timer_interrupt(unsigned long long seconds);
void                set_timer_interrupt_by_tick(unsigned long long tick);
unsigned long long  get_cpu_tick_plus_s(unsigned long long seconds);
void                set_alert_2S(char* str);
void                add_timer(void *callback, unsigned long long timeout, char* args);
void                timer_event_callback(timer_event_t * timer_event);
void                core_timer_handler();
void                timer_list_init();

#endif  /*_TIMER_H_ */