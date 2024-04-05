#ifndef TIMER_H
#define TIMER_H

extern void enable_core0_timer(void);
extern void disable_core0_timer(void);

unsigned int get_seconds(void);
void set_seconds(unsigned int s);
void set_core_timer_timeout(void);


#endif /* TIMER_H */
