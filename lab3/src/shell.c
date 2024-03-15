#include "shell.h"
#include "uart.h"
#include "string.h"
#include "cmd.h"

void run_shell()
{
    while (1) {
        char buffer[MAX_BUF_SIZE + 1];

        uart_puts("# ");
        read_user_input(buffer);
        exec_command(buffer);

        // Stop the shell if rebooting
        if (!strcmp(buffer, "reboot"))
            break;
    }
}

void read_user_input(char *buf)
{
    // FIXME: buffer overflow is not handled!

    int idx = 0;

    while (1) {
        char c = uart_getc();

        if (c == '\n') {
            uart_putc(c);
            buf[idx] = '\0';
            break;
        } else if (c >= 32 && c <= 126) {
            uart_putc(c);
            buf[idx++] = c;
        } else if (c == 127) {
            // Handle backspaces
            if (idx > 0) {
                buf[idx--] = 0;
                uart_putc('\b');
                uart_putc(' ');
                uart_putc('\b');
            }
        } else {
            // Ignore unprintable chars
            continue;
        }
    }
}

int exec_command(const char *command)
{
    int i = 0;

    while (1) {
        if (!strcmp(commands[i].name, "NULL")) {
            uart_puts("Command not found.\n");
            return -1;
        } else if (!strcmp(commands[i].name, command)) {
            commands[i].func();
            return 0;
        }
        i++;
    }
    return 0;
}