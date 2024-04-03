#ifndef __TIMER_H__
#define __TIMER_H__

#define CORE0_TIMER_IRQ_CTRL 0x40000040

extern void core_timer_enable();
extern void core_timer_disable();
extern unsigned long long get_cpu_cycles();
extern unsigned int get_cpu_freq();
extern void set_timer_asm(unsigned int sec);

void print_time_handler();
void set_timer(unsigned int sec);

#endif