#ifndef	_IRQ_H
#define	_IRQ_H



void enable_interrupt_controller( void );

void irq_vector_init( void );
void enable_irq( void );
void disable_irq( void );
void irq_except_handler_timer_c();

extern int timer_flag;
#endif  /*_IRQ_H */
