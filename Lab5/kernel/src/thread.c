#include "fork.h"
#include "mini_uart.h"
#include "sched.h"
#include "sprintf.h"
#include "string.h"
#include "sys.h"
#include "timer.h"
#include "utils.h"

void foo(void)
{
    for (int i = 0; i < 10; ++i) {
        uart_printf("Thread id: %d %d\n", current_task->pid, i);
        delay(1000000);
    }
    exit_process();
}

void test_thread(void)
{
    uart_printf("test_thread start\n");
    for (int i = 0; i < 3; i++)
        copy_process(PF_KTHREAD, foo, NULL);
}


void kernel_process(void* user_process)
{
    uart_printf("Kernel process started. EL %d\n", get_el());
    int err = move_to_user_mode((unsigned long)user_process);
    if (err < 0)
        uart_printf("Error while moving process to user mode\n");
}

void exec_test(void)
{
    uart_printf("\nExec Test, pid %d\n", getpid());
    exec("syscall.img", NULL);
    uart_printf("\nExec Fail\n");
    exit();
}

void fork_test(void)
{
    uart_printf("\nFork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) {  // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n",
                    getpid(), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0) {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n",
                        getpid(), cnt, &cnt, cur_sp);
        } else {
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n",
                            getpid(), cnt, &cnt, cur_sp);
                delay(1000000);
                ++cnt;
            }
        }
        exit();
    } else {
        uart_printf("parent here, pid %d, child %d\n", getpid(), ret);
    }

    exit();
}
