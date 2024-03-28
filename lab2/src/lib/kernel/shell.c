#include "shell.h"
#include "cpio.h"
#include "mbox.h"
#include "reboot.h"
#include "string.h"

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
            uart_puts("help\t: print this help menu\nhello\t: print Hello World!\nls\t: list files in current "
                      "directory\ncat\t: print file content\nmailbox\t: print mailbox info\nclear\t: clear the "
                      "screen\nreboot\t: reboot the device\n");
        else if (!strcmp(arg[0], "hello") && i == 1)
            uart_puts("Hello World!\n");
        else if (!strcmp(arg[0], "clear") && i == 1)
            uart_puts("\033[2J\033[H");
        else if (!strcmp(arg[0], "ls") && i == 1)
            cpio_ls();
        else if (!strcmp(arg[0], "cat") && i > 0) {
            if (i == 2)
                cpio_cat(arg[1]);
            else
                uart_puts("Usage: cat <filename>\n");
        }
        else if (!strcmp(arg[0], "mailbox") && i == 1) {
            get_board_revision();
            get_memory_info();
        }
        else if (!strcmp(arg[0], "reboot") && i == 1) {
            uart_puts("\033[2J\033[H");
            uart_puts("Rebooting...");
            reset(200);
        }
    }
}
