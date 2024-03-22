#include "shell.h"
#include "load.h"
#include "mbox.h"
#include "reboot.h"

int strcmp(const char *s1, const char *s2)
{
    const char *p1 = s1, *p2 = s2;
    while (*p1 && *p2) {
        if (*p1 != *p2)
            return 0;
        p1++;
        p2++;
    }
    return *p1 == *p2;
}

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
        char cmd[256];

        uart_puts("# ");
        read(cmd, 255);
        uart_puts("\n");
        if (strcmp(cmd, "hello")) {
            uart_puts("Hello World!\n");
        }
        else if (strcmp(cmd, "help")) {
            uart_puts("help\t: print this help menu\nhello\t: print Hello World!\nmailbox\t: print mailbox "
                      "info\nclear\t: clear the screen\nreboot\t: reboot the device\n");
        }
        else if (strcmp(cmd, "reboot")) {
            uart_puts("Rebooting...\n");
            uart_puts("\033[2J\033[H");
            reset(200);
        }
        else if (strcmp(cmd, "mailbox")) {
            get_board_revision();
            get_memory_info();
        }
        else if (strcmp(cmd, "clear")) {
            uart_puts("\033[2J\033[H");
        }
    }
}

void bootloader_shell()
{
    while (1) {
        char cmd[256];

        uart_puts("# ");
        read(cmd, 255);
        uart_puts("\n");
        if (strcmp(cmd, "hello")) {
            uart_puts("Hello World!\n");
        }
        else if (strcmp(cmd, "help")) {
            uart_puts("help\t: print this help menu\nhello\t: print Hello World!\nload\t: load kernel image through "
                      "uart\nclear\t: clear the screen\nreboot\t: reboot the device\n");
        }
        else if (strcmp(cmd, "reboot")) {
            uart_puts("Rebooting...\n");
            uart_puts("\033[2J\033[H");
            reset(200);
        }
        else if (strcmp(cmd, "load")) {
            load();
        }
        else if (strcmp(cmd, "clear")) {
            uart_puts("\033[2J\033[H");
        }
    }
}