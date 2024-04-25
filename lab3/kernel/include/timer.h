#ifndef	_TIMER_H_
#define	_TIMER_H_

#define CORE0_TIMER_IRQ_CTRL 0x40000040


void core_timer_enable();
void set_time_out(unsigned long long seconds);

#endif  /*_TIMER_H_ */