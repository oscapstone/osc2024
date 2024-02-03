#include "sched.h"
#include "uart.h"
#include "exec.h"
#include "syscall.h"

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
        wait_sec(1);
    }
}

void timer_task2()
{
    while (1) {
        uart_puts("kernel timer task2.\n");
        wait_sec(1);
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
    long count = 1000000000000000;
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