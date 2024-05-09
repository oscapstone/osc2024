#ifndef __TIME_C_H
#define __TIME_C_H

#define CORE0_TIMER_IRQ_CTRL	((volatile unsigned int*)(0x40000040))
#define CORE0_INTERRUPT_SOURCE	((volatile unsigned int*)(0x40000060))

typedef void (*task_callback)();

typedef struct timer_info {
    struct timer_info *prev;
    struct timer_info *next;
    char* msg;
    task_callback callback;
    unsigned long long executeTime;
}timer_info;

extern timer_info *timer_head;

void setTimeout(char* message, unsigned long long wait);
void timer_init();

#endif