#include "uart.h"
#include "reboot.h"
#include "utils.h"
#include "cpio.h"
#include "timer.h"
#include "allocator.h"
#include "exception.h"
#include "schedule.h"
#include "syscall.h"
#include "vfs.h"
#include "shell.h"

#define buf_size 1024

void exec_syscall_img()
{
    do_exec("vm.img", NULL);
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
    else if (my_strcmp(cmd, "timer") == 0)
    {
        uart_puts("\n");
        periodic_timer(0, 0);
        while (1)
        {
            char c = uart_read();
            if (c == '\n')
            {
                core_timer_disable();
                while (timer_hp->size > 0)
                    timer_heap_extractMin(timer_hp);
                core_timer_enable();
                break;
            }
        }
    }
    else if (my_strcmp(cmd, "exc") == 0)
    {
        uart_puts("\n");
        uart_puts("exc user program\n");

        // set EL0PCTEN to 1, EL0 can accesses to the frequency register and physical counter register without trap.
        unsigned long long tmp;
        asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
        tmp |= 1;
        asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));

        unsigned long long cntpct_el0 = 0;
        asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0)); // get timer’s current count.
        unsigned long long cntfrq_el0 = 0;
        asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0)); // get timer's frequency

        task_create(exec_syscall_img, 0);

        timer temp;
        temp.callback = re_shedule;
        temp.expire = cntpct_el0 + (cntfrq_el0 >> 5);
        add_timer(temp);

        zombie_reaper();
    }
    else if (my_strcmp(cmd, "vfs") == 0)
    {
        uart_puts("\n");
        char buf[200];
        struct file *a = vfs_open("/hello", O_CREAT);
        struct file *b = vfs_open("/world", O_CREAT);
        vfs_write(a, "Hello ", 6);
        vfs_write(b, "World!", 6);
        vfs_close(a);
        vfs_close(b);
        b = vfs_open("/hello", 0);
        a = vfs_open("/world", 0);
        int sz;
        sz = vfs_read(b, buf, 100);
        sz += vfs_read(a, buf + sz, 100);
        buf[sz] = '\0';
        uart_puts(buf);
        uart_puts("\n");
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