#include "uart.h"
#include "reboot.h"
#include "utils.h"
#include "cpio.h"
#include "timer.h"
#include "allocator.h"
#include "exception.h"
#include "shell.h"

#define buf_size 1024

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
    else if (my_strcmp(cmd, "asyn") == 0)
    {
        uart_puts("\n");
        while (1)
            uart_asyn_write(uart_asyn_read());
    }
    else if (my_strncmp(cmd, "setTimeout", 10) == 0)
    {
        uart_puts("\n");
        char *message_start = cmd + 11, *message_end = cmd + 11;
        while (*message_end != ' ')
            message_end++;

        int size = message_end - message_start + 1;
        char *message = kmalloc(sizeof(char) * size); // malloc for message
        for (int i = 0; i < size; i++)
            message[i] = message_start[i];
        message[size - 1] = '\0';

        int second = atoi(message_end + 1);
        setTimeout(message, second);
    }
    else if (my_strcmp(cmd, "test preemption") == 0)
    {
        uart_puts("\n");
        test_preemption();
        int count = 100000; // wait for interrupt
        while (count--)
            asm volatile("nop\n");
    }
    else if (my_strcmp(cmd, "test block") == 0)
    {
        uart_puts("\n");
        print_free_area();
        char *ptr = kmalloc(5000);
        ptr = kmalloc(5000);
        ptr = kmalloc(4000);
        uart_puts("-----------------------------------------------------\n");
        uart_puts("-----------------------------------------------------\n");
        kfree(ptr);
    }
    else if (my_strcmp(cmd, "test object") == 0)
    {
        uart_puts("\n");
        print_free_object();
        char *ptr_arr[64];

        for (int i = 0; i < 65; i++)
            ptr_arr[i] = kmalloc(60);

        uart_puts("-----------------------------------------------------\n");
        uart_puts("-----------------------------------------------------\n");
        
        kfree(ptr_arr[50]);
        kfree(ptr_arr[0]);
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