#ifndef _TIMER_H_
#define _TIMER_H_

#define CORE0_TIMER_IRQ_CTRL 0x40000040

void core_timer_enable(unsigned long long expired_time);
void core_timer_disable();
void core_timer_handler();

#endif /* _TIMER_H_ */
