#include "../include/thread.h"
#include "../include/sched.h"
#include "../include/mini_uart.h"
#include "../include/list.h"

extern struct task_struct *current;

void foo(void)
{
    for (int i = 0; i < 10; i++) {
        printf("Thread id: %d %d\n", current->pid, i);
        delay(1000000);
        schedule();
    }

    exit_process();
}