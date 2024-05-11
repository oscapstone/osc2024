#include "uart.h"
#include "reboot.h"
#include "utils.h"
#include "cpio.h"
#include "timer.h"
#include "allocator.h"
#include "exception.h"
#include "schedule.h"
#include "syscall.h"
#include "shell.h"

#define buf_size 1024

void fork_test()
{
    uartwrite("\nFork Test, pid ", my_strlen("\nFork Test, pid "));
    uart_dec(getpid());
    uartwrite("\n", 1);

    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0)
    { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));

        uart_puts("first child pid:");
        uart_dec(getpid());
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
            uart_dec(getpid());
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
                uart_dec(getpid());
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
        exit(0);
        uart_puts("Should not be printed\n");
    }
    else
    {
        uart_puts("parent here, pid ");
        uart_dec(getpid());
        uart_puts(", child: ");
        uart_dec(ret);
        uart_puts("\n");
        exit(0);
        uart_puts("Should not be printed\n");
    }
    exit(0);
}

void user_goo()
{
    do_exec(fork_test);
}

void kernel_tese()
{
    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0)); // get timer’s current count.
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0)); // get timer's frequency

    timer temp;
    temp.callback = re_shedule;
    temp.expire = cntpct_el0 + (cntfrq_el0 >> 5);
    add_timer(temp);

    task_create(user_goo, 0);
    zombie_reaper();
}

void shell_cmd(char *cmd)
{
    if (my_strcmp(cmd, "help") == 0)
    {
        uart_puts("\n");
        uart_puts("help     : print all available commands\n");
        uart_puts("hello    : print Hello World!\n");
        uart_puts("reboot   : reboot the device\n");
    }
    else if (my_strcmp(cmd, "hello") == 0)
    {
        uart_puts("\n");
        uart_puts("Hello World!\n");
    }
    else if (my_strcmp(cmd, "reboot") == 0)
    {
        uart_puts("\n");
        uart_puts("rebooting...\n");
        reset(500);
        while (1)
            ;
    }
    else if (my_strcmp(cmd, "ls") == 0)
    {
        uart_puts("\n");
        traverse_file();
    }
    else if (my_strncmp(cmd, "cat", 3) == 0)
    {
        uart_puts("\n");
        char *pathname = cmd + 4;
        while (*pathname == '\0' || *pathname == ' ')
            pathname++;
        look_file_content(pathname);
    }
    else if (my_strcmp(cmd, "exc") == 0)
    {
        uart_puts("\n");
        uart_puts("exc user program\n");
        exec_program("user.img");
    }
    else if (my_strcmp(cmd, "test syscall") == 0)
    {
        uart_puts("\n");
        kernel_tese();
    }
    else
        uart_puts("\n");
}

void simple_shell()
{
    uart_puts("# ");
    char cmd[buf_size];
    cmd[0] = '\0';
    int idx = 0, len = 0;
    while (1)
    {
        char c = uart_read();
        if (c == '\n') // When enter new line, call the command in cmd array
        {
            shell_cmd(cmd);
            uart_puts("# ");
            idx = 0;
            len = 0;
            cmd[idx] = '\0';
        }
        else if (c == '\e') // ANSI escape
        {
            if ((c = uart_read()) == '[')
            {
                if ((c = uart_read()) == 'C' && idx < len) // Cursor Forward
                {
                    idx++;
                    uart_puts("\e[C");
                }
                else if (c == 'D' && idx > 0) // Cursor Backward
                {
                    idx--;
                    uart_puts("\e[D");
                }
            }
        }
        else if (c == 8 || c == 127) // Backspace
        {
            if (idx > 0)
            {
                idx--;
                len--;
                for (int i = idx; i < buf_size; i++)
                    cmd[i] = cmd[i + 1];
                if (idx == len)
                {
                    uart_puts("\e[D");
                    uart_write(' ');
                    uart_puts("\e[D");
                }
                else if (idx < len)
                {
                    uart_puts("\e[D");
                    uart_puts(cmd + idx);
                    uart_write(' ');
                    uart_puts("\e[D");
                    int count = len - idx;
                    while (count-- > 0)
                        uart_puts("\e[D");
                }
            }
        }
        else
        {
            if (idx < buf_size) // Print the new character
            {
                for (int i = buf_size - 1; i > idx; i--)
                    cmd[i] = cmd[i - 1];
                cmd[idx] = c;
                uart_puts(cmd + idx);
                idx++;
                len++;
                if (idx < len)
                {
                    int count = len - idx;
                    while (count-- > 0)
                        uart_puts("\e[D");
                }
            }
        }
    }
}