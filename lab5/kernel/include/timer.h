#ifndef _TIMER_H_
#define _TIMER_H_

#include "list.h"
#include "stdint.h"

#define CORE0_TIMER_IRQ_CTRL 0x40000040

void core_timer_enable();
void core_timer_disable();
void core_timer_handler();

typedef struct timer_event {
    struct list_head listhead;
    uint64_t interrupt_time;  //store as tick time after cpu start
    void *callback; // interrupt -> timer_callback -> callback(args)
    char* args_struct; // need to free the string by event callback function
} timer_event_t;

//now the callback only support "funcion(char *)", char* in args
void timer_event_callback(timer_event_t * timer_event);
void add_timer(void *callback, uint64_t timeout, void *args_struct);
uint64_t get_tick_plus_s(uint64_t second);
void set_core_timer_interrupt(uint64_t expired_time);
void set_core_timer_interrupt_by_tick(uint64_t tick);
void timer_set2sAlert();
void timer_list_init();
int  timer_list_get_size();


#endif /* _TIMER_H_ */
