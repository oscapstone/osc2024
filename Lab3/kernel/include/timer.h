#ifndef TIMER_H
#define TIMER_H

extern void enable_core0_timer(void);
extern void disable_core0_timer(void);
extern unsigned long get_current_time(void);

unsigned int get_seconds(void);
void set_seconds(unsigned int s);
void set_core_timer_timeout(void);


typedef void (*timer_callback)(char* msg);

void add_timer(timer_callback cb, char* msg, unsigned int duration);

void core_timer_handle_irq(void);

void set_timeout(char* msg, unsigned int duration);


#endif /* TIMER_H */
