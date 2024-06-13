#ifndef	_EXCEPTION_H
#define	_EXCEPTION_H

#include "peripherals/base.h"

#ifndef __ASSEMBLER__

void sync ( void );
void irq_from_el0 ( void );
void irq_from_el1( void );
void undefined( void );

void enable_aux_interrupt();
void disable_aux_interrupt();

void enable_el1_interrupt();
void disable_el1_interrupt();

#endif

#endif
