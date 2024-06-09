#include "fork.h"
#include "mini_uart.h"
#include "sched.h"
#include "timer.h"
#include "utils.h"

void foo(void)
{
    for (int i = 0; i < 10; ++i) {
        uart_printf("Thread id: %d %d\n", current_task->pid, i);
        delay(1000000);
    }
    kill_task(current_task, 0);
}

void test_thread(void)
{
    uart_printf("test_thread start\n");
    for (int i = 0; i < 3; i++)
        copy_process(&foo, NULL);
}
