#ifndef CURRENT_H
#define CURRENT_H

#include "sched.h"

// tpidr_el1 :  64 bit system register
// get current thread data structure
// msb 2 bit : thread id
static inline thread_context_t *get_current_ctx(void)
{
    thread_context_t *cur;
    __asm__ __volatile__(
        "mrs %0, tpidr_el1\n\t"
        : "=r"(cur));
    return cur;
}

static inline void set_current_ctx(thread_context_t *thread_context)
{
    __asm__ __volatile__(
        "msr tpidr_el1, %0\n\t" 
        ::"r"(thread_context));
}

#define current_ctx get_current_ctx()

#endif