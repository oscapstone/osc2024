#ifndef	_TIMER_H_
#define	_TIMER_H_

#define CORE0_TIMER_IRQ_CTRL 0x40000040


void                core_timer_enable();
void                core_timer_disable();
void                set_timer_interrupt(unsigned long long seconds);
void                set_timer_interrupt_by_tick(unsigned long long tick);
unsigned long long  get_cpu_tick_plus_s(unsigned long long seconds);
void                set_alert_2S(char* str);


#endif  /*_TIMER_H_ */