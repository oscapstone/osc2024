#ifndef _DEF_INTERRUPT
#define _DEF_INTERRUPT


#define CORE0_TIMER_IRQ_CTRL 0x40000040
void init_core_timer(void);
void handle_interrupt(void);

#endif
