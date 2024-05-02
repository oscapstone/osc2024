#ifndef _IRQ_H_
#define _IRQ_H_

#include <stdint.h>

void irq_vector_init(void);
void enable_el1_interrupt(void);
void disable_el1_interrupt(void);
void handle_sync_el0_64(void);
void irq_el0_64(void);
void handle_sync_el1h(void);

#endif