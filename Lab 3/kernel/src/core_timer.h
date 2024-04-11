#ifndef __CORE_TIMER_H__
#define __CORE_TIMER_H__

#include "type.h"

#define CORE0_TIMER_IRQ_CTRL    ((volatile uint32ptr_t) (0x40000040))
#define CORE0_INTERRUPT_SOURCE  ((volatile uint32ptr_t) (0x40000060))

void core_timer_enable(uint32_t duration);
void core_timer_disable();

void core_timer_set_alarm(uint32_t second);
void core_timer_interrupt_handler();
void core_timer_set_timeout(byteptr_t message, uint64_t duration);

#endif