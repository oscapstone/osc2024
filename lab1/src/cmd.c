#include "cmd.h"
#include "string.h"
#include "uart.h"

struct command commands[] = {
	{ .name = "help", .help = "print this help menu", .func = help },
	{ .name = "hello", .help = "print Hello World!", .func = hello },
	{ .name = "reboot", .help = "reboot the device", .func = reboot },
	{ .name = "NULL" } // Must put a NULL command at the end!
};

void help()
{
	int i = 0;

	while (1) {
		if (!strcmp(commands[i].name, "NULL")) {
			break;
		}
		uart_puts(commands[i].name);
		uart_puts("\t: ");
		uart_puts(commands[i].help);
		uart_putc('\n');
		i++;
	}
}

void hello()
{
	uart_puts("Hello World!\n");
}

void reboot()
{
#define PM_PASSWORD 0x5A000000
#define PM_RSTC (volatile unsigned int *)0x3F10001C
#define PM_WDOG (volatile unsigned int *)0x3F100024
	// Reboot after 180 ticks
	*PM_RSTC = PM_PASSWORD | 0x20; // Full reset
	*PM_WDOG = PM_PASSWORD | 180;  // Number of watchdog ticks
}