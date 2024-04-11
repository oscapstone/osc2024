#include "sched.h"
#include "uart.h"
#include "exec.h"
#include "syscall.h"
#include "delays.h"
#include "exception.h"
#include "interrupt.h"
#include "timer.h"


void demo_task1()
{
    while (1) {
        uart_puts("demo task1\n");
        wait_sec(1);
        schedule();
        // context_switch(&task_pool[2]); // for requirement 1-3
    }
}

void demo_task2()
{
    while (1) {
        uart_puts("demo task2\n");
        wait_sec(1);
        schedule();
        // context_switch(&task_pool[1]); // for requirement 1-3
    }
}

void timer_task1()
{
    while (1) {
        uart_puts("kernel timer task1.\n");
        wait_sec(10);
    }
}

void timer_task2()
{
    while (1) {
        uart_puts("kernel timer task2.\n");
        wait_sec(10);
    }
}

void user_task1()
{
    while (1) {
        uart_puts("user task1\n");
        int count = 1000000000;
        while (count--);
    }
}

void user_task2()
{
    while (1) {
        uart_puts("user task2\n");
        int count = 1000000000;
        while (count--);
    }
}

void demo_do_exec1()
{
    do_exec(user_task1);
}

void demo_do_exec2()
{
    do_exec(user_task2);
}

void delay()
{
    long count = 1000000; // 1000000000 can see the effect of multi-tasking, or I should increase the frequency of timer interrupt.
    while (count--);
}

void foo(){
    int tmp = 5;
    printf("Task %d after exec, tmp address 0x%x, tmp value %d\n", get_taskid(), &tmp, tmp);
    exit(0);
}

/* At user space, not allowed to access timer */
void test() {
    printf("Into test()\n");
    int cnt = 1;
    if (fork() == 0) {
        fork();
        delay();
        fork();
        while(cnt < 10) {
            printf("Task id: %d, cnt: %d\n", get_taskid(), cnt);
            delay();
            ++cnt;
        }
        printf("Child task %d done before exit\n", get_taskid());
        exit(0);
        printf("Should not be printed\n");
    } else {
        printf("Task %d before exec, cnt address 0x%x, cnt value %d\n", get_taskid(), &cnt, cnt);
        exec(foo);
    }
}

void user_test()
{
    do_exec(test);
}

/* Lab 3 Basic Exercise 3: Asynchronous UART */
void demo_async_uart()
{
    char string[64];
    int len;

    uart_async_init(); // Initialize the Asynchronous UART
    uart_async_puts("Demo lab 3: Asynchronous UART\n"); // Put the string to write_buffer
    wait_cycles(150);
    while (1) {
        len = uart_async_gets(string); // Get all data in read_buffer
        wait_cycles(150);
        if (len > 0) {
            uart_hex(len); // output the length of the string
            uart_async_puts("\n");
            uart_async_puts(string); // output the string in read_buffer
            uart_async_puts("\n");
        }
        wait_cycles(10000);
    };
}

/* Lab3 Advanced 2: bottom half irq */
void demo_bh_irq(void)
{
    enable_interrupt(); /* In this demo, we are in el1. So enable interrupt in el1 first. */
    core_timer_enable();
    uart_async_init();
    uart_puts("Demo Lab3 Advanced 2: bottom half irq\n");
    while (1) {
        do_tasklet();
        // wait_cycles(1000000000);
    }
}