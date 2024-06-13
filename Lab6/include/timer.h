#ifndef _TIMER_H_
#define _TIMER_H_

#include "list.h"
#include "bcm2837/rpi_mmu.h"

#define CORE0_TIMER_IRQ_CTRL PHYS_TO_VIRT(0x40000040)



typedef struct timer_event {
    struct list_head listhead;
    unsigned long long interrupt_time;  //store as tick time after cpu start
    void *callback; // interrupt -> timer_callback -> callback(args)
    char* args; // need to free the string by event callback function
} timer_event_t;

extern struct list_head *timer_list;

//now the callback only support "funcion(char *)", char* in args
void timer_callback(timer_event_t * timer_event);
void core_timer_enable();
void core_timer_disable();
void core_timer_handler();
void add_timer(void *callback, unsigned long long timeout, char* args, int tick);
unsigned long long get_time_add_sec(unsigned long long second);
void set_core_timer_interrupt(unsigned long long expired_time);
void set_timer_interrupt(unsigned long long sec);
//void timer_2s(char* str);
void timer_list_init();
int  timer_list_get_size();


#endif /* _TIMER_H_ */
