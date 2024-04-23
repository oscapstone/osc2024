#ifndef _DEF_EXCEPTION
#define _DEF_EXCEPTION

#define CORE0_TIMER_IRQ_CTRL 0x40000040
#define CORE0_TIMER_IRQ_SRC  (unsigned int *) 0x40000060


extern int __userspace_start;
extern int __userspace_end;

void init_exception_vectors(void);
void init_interrupt(void);
void _init_core_timer(void);

void handle_exception(void);
void handle_interrupt(void);
void handle_current_el_irq(void);
void _handle_timer(void);

// exception level
void el2_to_el1(void);
void el1_to_el0(void);

void enable_interrupt(void);
void disable_interrupt(void);

#endif
