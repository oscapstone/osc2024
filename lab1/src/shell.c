#include "shell.h"
#include "uart.h"
#include "string.h"
#include "cmd.h"

#define MAX_BUF_SIZE 256

void run_shell()
{
	// Warning: buffer overflow not handled!

	while (1) {
		uart_puts("# ");

		// Get user inputs
		char buf[MAX_BUF_SIZE + 1];
		int idx = 0;

		while (1) {
			char c = uart_getc();
			uart_putc(c);

			if (c == '\n') {
				buf[idx] = '\0';
				break;
			} else if (c == 127) {
				// Handle backspaces
				if (idx > 0) {
					buf[idx--] = 0;
					uart_putc('\b');
					uart_putc(' ');
					uart_putc('\b');
				}
			} else {
				buf[idx++] = c;
			}
		}

		exec_command(buf);
	}
}

int exec_command(const char *input)
{
	int i = 0;

	while (1) {
		if (!strcmp(commands[i].name, "NULL")) {
			uart_puts("Command not found.\n");
			return -1;
		} else if (!strcmp(commands[i].name, input)) {
			commands[i].func();
			return 0;
		}
		i++;
	}
	return 0;
}