#ifndef _TIMER_H
#define _TIMER_H

#define CORE0_TIMER_IRQ_CTRL 0x40000040

typedef void (*timer_callback_t)(void *data, unsigned int timeout);
typedef struct timer {
    struct timer *prev;
    struct timer *next;
    timer_callback_t callback;
    void *data;
    unsigned int timeout;
    unsigned long long expired_time;
} timer_t;

void add_timer(timer_callback_t callback, void *data, unsigned int timeout);
void insert_timer_list(timer_t *timer);
void set_timeout(const char* message, unsigned int timeout);
void print_message(void *data, unsigned int timeout);

#endif