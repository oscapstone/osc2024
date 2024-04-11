#include "shell.h"
#include "cpio.h"
#include "exception.h"
#include "mbox.h"
#include "reboot.h"
#include "string.h"
#include "timer.h"

void read(char *buf, int len)
{
    int i = 0;
    while (i < len) {
        char c = uart_getc();
        if (c == '\n') {
            buf[i] = '\0';
            return;
        }
        else if (c == 127) {
            if (i > 0) {
                i--;
                uart_puts("\b \b");
            }
        }
        else {
            buf[i++] = c;
            uart_send(c);
        }
    }
    buf[i] = '\0';
}

void shell()
{
    while (1) {
        char cmd[256], arg[4][64], *token;

        uart_puts("# ");
        read(cmd, 255);
        uart_puts("\n");

        // parse command
        int i = 0;
        token = strtok(cmd, " ");
        while (token != 0) {
            strcpy(arg[i++], token);
            token = strtok(0, " ");
        }

        if (!strcmp(arg[0], "help") && i == 1)
            uart_puts(
                "help\t: print this help menu\nhello\t: print Hello World!\nls\t: list files in current "
                "directory\ncat\t: print file content\nexec\t: execute user program\nasync\t: test async\ntimeout\t: "
                "set timeout\nmailbox\t: print mailbox info\nclear\t: clear the screen\nreboot\t: reboot the device\n");
        else if (!strcmp(arg[0], "hello") && i == 1)
            uart_puts("Hello World!\n");
        else if (!strcmp(arg[0], "clear") && i == 1)
            uart_puts("\033[2J\033[H");
        else if (!strcmp(arg[0], "ls") && i == 1)
            cpio_ls();
        else if (!strcmp(arg[0], "cat")) {
            if (i == 2)
                cpio_cat(arg[1]);
            else
                uart_puts("Usage: cat <filename>\n");
        }
        else if (!strcmp(arg[0], "exec") && i == 1)
            cpio_exec("userprog");
        else if (!strcmp(arg[0], "async") && i == 1) {
            // uart_interrupt_enable();
            // uart_puts("Press enter to exit\n");
            uart_puts("async (press enter to exit)>> ");
            char c = 0;
            while (1) {
                c = uart_async_getc();
                if (c == 13 || c == 10) {
                    uart_clear_buffers();
                    break;
                }
                else if (c > 127)
                    continue;
                uart_async_putc(c);
            }
            uart_puts("\n");
        }
        // set timeout
        else if (!strcmp(arg[0], "timeout")) {
            if (i != 3)
                uart_puts("Usage: timeout <message> <time>\n");
            else {
                unsigned long long cntpct, cntfrq;
                asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct));
                asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
                add_timer(uart_puts, arg[1], atoi(arg[2]) * cntfrq + cntpct);
            }
        }
        else if (!strcmp(arg[0], "preempt") && i == 1)
            test_preemption();
        else if (!strcmp(arg[0], "mailbox") && i == 1) {
            get_board_revision();
            get_memory_info();
        }
        else if (!strcmp(arg[0], "reboot") && i == 1) {
            uart_puts("\033[2J\033[H");
            uart_puts("Rebooting...");
            reset(200);
        }

        // clear arguments
        for (int j = 0; j < i; j++)
            arg[j][0] = '\0';
    }
}
