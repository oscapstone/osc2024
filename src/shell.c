#include "shell.h"
#include "string.h"
#include "uart.h"

void shell_start()
{
    uart_puts("# ");

    char line[MAX_GETLINE_LEN];
    char c;
    unsigned int index = 0;
    while (1) {
        c = (char) uart_getc();

        if (c == '\n') {
            line[index++] = '\0';
            uart_puts("\n");

            if (strcmp(line, "help") == 0) {
                uart_puts("help\t: print this help menu\nhello\t: print Hello World!\nreboot\t: reboot the device\n");
            } else if (strcmp(line, "hello") == 0) {
                uart_puts("Hello World!\n");
            } else if (strcmp(line, "reboot") == 0) {
                uart_puts("Reboot!\n");
            } else {
                uart_puts("command not found\n");
            }

            index = 0;
            uart_puts("# ");
        } else {
            line[index++] = c;
            uart_send(c);
        }
    }
}