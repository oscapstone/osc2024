#ifndef _TIMER_H
#define _TIMER_H

#define CORE0_TIMER_IRQ_CTRL ((volatile unsigned int *)(0x40000040))

typedef void (*timer_callback_t)(void *data);

typedef struct timer {
    struct timer *next;
    timer_callback_t callback;
    void *data;
    unsigned long long timeout;
} timer_t;

void set_timeout(char* message, unsigned long long timeout);
void print_message(void *data);
void create_timer(
    timer_callback_t callback, 
    void *data, 
    unsigned long long timeout
);
void create_timer_freq_shift(
    timer_callback_t callback, 
    void *data, 
    unsigned long long shift
);
void add_timer(timer_t **timer);
void schedule_task(void* data);

#endif