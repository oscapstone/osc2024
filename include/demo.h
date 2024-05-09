#ifndef __DEMO_H__
#define __DEMO_H__

// #define DEMO // This macro is for demo tasklet (nested interrupt) for now.

void demo_async_uart(void);
void demo_bh_irq(void);

void demo_memory_allocator(void);

void fork_test(void);
void demo_fork_test(void);


#endif