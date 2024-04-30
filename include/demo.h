#ifndef __DEMO_H__
#define __DEMO_H__

// #define DEMO // This macro is for demo tasklet (nested interrupt) for now.

void demo_task1(void);
void demo_task2(void);
void timer_task1(void);
void timer_task2(void);
void user_task1(void);
void user_task2(void);

void demo_do_exec1(void);
void demo_do_exec2(void);

void user_test(void);

void demo_async_uart(void);
void demo_bh_irq(void);

void demo_memory_allocator(void);
void demo_fork_test(void);


#endif