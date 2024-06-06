#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
// #include <stddef.h>

#ifdef _cplusplus
#define NULL 0
#else
#define NULL (void*)0
#endif

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ long unsigned int
#endif
// #if !(defined (__GNUG__) && defined (size_t))
typedef __SIZE_TYPE__ size_t;

typedef void (*timer_callback)(void *data);

typedef struct timer {
    struct timer *prev;  // previous timer in the list
    struct timer *next;  // next timer in the list
    timer_callback callback;  // the function to call when the timer expires
    void *data;  // data to be passed to the callback
    uint64_t expiry;  // the time at which the timer will expire
} timer_t;

extern timer_t *timer_head;  // head of the timer list

void setTimeout(char *message,uint64_t seconds);
#endif

