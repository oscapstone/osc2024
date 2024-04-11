#include <stddef.h>

#include "shell.h"
#include "uart.h"
#include "string.h"
#include "reset.h"
#include "mbox.h"
#include "cpio.h"

static void dispatch_cmd(const char *cmd)
{
	if (strcmp(cmd, "hello") == 0) {
		uart_puts("Hello, world!\r\n");
		return;
	}
	if (strcmp(cmd, "help") == 0) {
		uart_puts("Available commands:\r\n");
		uart_puts("  hello - print a greeting\r\n");
		uart_puts("  help - print this help message\r\n");
		uart_puts("  reboot - reboot the system\r\n");
		uart_puts(
			"  info - print the board revision, memory base address, memory size, and serial number\r\n");
		uart_puts("  ls - List information about the FILEs\r\n");
		uart_puts("  cat - Concatenate FILE(s)\r\n");
		return;
	}
	if (strcmp(cmd, "reboot") == 0) {
		uart_puts("rebooting...\r\n");
		reset(1000);
		do {
			asm volatile("nop");
		} while (1);
	}
	if (strcmp(cmd, "info") == 0) {
		get_board_revision();
		if (mbox_call(MBOX_CH_PROP)) {
			uart_puts("Revision: ");
			uart_puts("0x");
			uart_hex(mbox[5]);
			uart_puts("\r\n");
		} else {
			uart_puts("Unable to query revision!\n");
		}
		get_arm_mem();
		if (mbox_call(MBOX_CH_PROP)) {
			uart_puts("Memory base address: ");
			uart_puts("0x");
			uart_hex(mbox[5]);
			uart_puts("\r\n");
			uart_puts("Memory size: ");
			uart_puts("0x");
			uart_hex(mbox[6]);
			uart_puts("\r\n");
		} else {
			uart_puts("Unable to query memory info!\n");
		}
		get_serial_number();
		if (mbox_call(MBOX_CH_PROP)) {
			uart_puts("Serial Number: ");
			uart_puts("0x");
			uart_hex(mbox[6]);
			uart_hex(mbox[5]);
			uart_puts("\r\n");
		} else {
			uart_puts("Unable to query serial!\n");
		}
		return;
	}
	if (strcmp(cmd, "ls") == 0) {
		cpio_ls();
		return;
	}
	if (strcmp(cmd, "cat") == 0) {
		cpio_cat();
		return;
	}

	uart_puts("ERROR: Unrecognized command!\r\n");
}

void shell()
{
	char buffer[MAX_BUFFER_LEN];
	size_t buffer_index = 0;
	char ch;

	// set up serial console
	uart_init();

	// say hello
	uart_puts("Starting the shell!\r\n");

	while (1) {
		uart_puts("# ");
		buffer_index = 0;
		buffer[0] = '\0';

		while (1) {
			ch = uart_getc();
			if (buffer_index + 1 >= sizeof(buffer)) {
				uart_puts(
					"ERROR: Exceed buffer length limit!\r\n");
				buffer_index = 0;
				buffer[0] = '\0';
				break;
			}
			if (ch == '\n') {
				uart_puts("\r\n");
				buffer[buffer_index] = '\0';
				break;
			}
			uart_send(ch);
			buffer[buffer_index++] = ch;
		}
		dispatch_cmd(buffer);
	}

	// echo everything back
	while (1) {
		uart_send(uart_getc());
	}
}
