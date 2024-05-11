#include "cpio.h"
#include "allocator.h"
#include "utils.h"
#include "kernel_task.h"
#include "syscall.h"

void do_exec(void (*func)(void))
{
    task_struct *cur = get_current_task();
    asm volatile(
        "msr sp_el0, %0\n"
        "msr elr_el1, %1\n"
        "mov x10, 0b101\n"
        "msr spsr_el1, x10\n"
        "eret\n"
        :
        : "r"(cur->ustack), "r"(func));
}

/*void fork_test()
{
    uart_puts("\nFork Test, pid ");
    uart_dec(get_pid());
    uart_puts("\n");

    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0)
    { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));

        uart_puts("first child pid:");
        uart_dec(get_pid());
        uart_puts(", cnt: ");
        uart_dec(cnt);
        uart_puts(", ptr: ");
        uart_hex_lower_case(&cnt);
        uart_puts(", sp: ");
        uart_hex_lower_case(cur_sp);
        uart_puts("\n");

        ++cnt;

        if ((ret = fork()) != 0)
        {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_puts("first child pid:");
            uart_dec(get_pid());
            uart_puts(", cnt: ");
            uart_dec(cnt);
            uart_puts(", ptr: ");
            uart_hex_lower_case(&cnt);
            uart_puts(", sp: ");
            uart_hex_lower_case(cur_sp);
            uart_puts("\n");
        }
        else
        {
            while (cnt < 5)
            {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_puts("second child pid:");
                uart_dec(get_pid());
                uart_puts(", cnt: ");
                uart_dec(cnt);
                uart_puts(", ptr: ");
                uart_hex_lower_case(&cnt);
                uart_puts(", sp: ");
                uart_hex_lower_case(cur_sp);
                uart_puts("\n");

                int count = 1000000;
                while (count--)
                    asm volatile("nop");
                ++cnt;
            }
        }
        exit();
        uart_puts("Should not be printed\n");
    }
    else
    {
        uart_puts("parent here, pid ");
        uart_dec(get_pid());
        uart_puts(", child: ");
        uart_dec(ret);
        uart_puts("\n");
    }
}*/