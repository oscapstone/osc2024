#ifndef __CORE_TIMER_H__
#define __CORE_TIMER_H__

#include "type.h"

#define CORE0_TIMER_IRQ_CTRL    ((volatile uint32ptr_t) (0x40000040))
#define CORE0_INTERRUPT_SOURCE  ((volatile uint32ptr_t) (0x40000060))

uint64_t core_timer_current_time();
void core_timer_enable();
void core_timer_disable();

void core_timer_interrupt_handler();
void core_timer_add_timeout_event(byteptr_t message, uint64_t duration);

typedef void (*timer_event_cb) (byteptr_t);

void core_timer_add_tick(timer_event_cb cb, byteptr_t msg, uint64_t duration);

#endif