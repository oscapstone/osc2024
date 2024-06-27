#include "../include/thread.h"
#include "../include/sched.h"
#include "../include/mini_uart.h"
#include "../include/list.h"
#include "../include/sys.h"

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

void fork_test(void)
{
    printf("\nFork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp": "=r"(cur_sp));
        printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0) {
            asm volatile("mov %0, sp": "=r"(cur_sp));
            printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        }
        else {
            while (cnt < 5) {
                asm volatile("mov %0, sp":"=r"(cur_sp));
                printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
                // delay(1000000);
                ++cnt;
            }
        }
        exit();
    }
    else {
        printf("parent here, pid %d, child %d\n", getpid(), ret);
    }
    exit();
}