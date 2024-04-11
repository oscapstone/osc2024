#include <stddef.h>

#include "uart.h"
#include "string.h"

#define MAX_BUFFER_LEN 1024

void bootloader_main(void *dtb)
{
	char buffer[MAX_BUFFER_LEN];
	size_t buffer_index = 0;
	char ch;

	uart_init();
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
		// dispatch_cmd(buffer);

		if (strcmp(buffer, "boot") == 0) {
			unsigned int size = 0;
			unsigned char *size_buffer = (unsigned char *)&size;

			uart_puts("Use send_loader.py to load kernel\r\n");
			for (int i = 0; i < 4; i++)
				size_buffer[i] = uart_getc();
			uart_puts("Start receiving kernel\r\n");

			char *kernel = (char *)0x80000; // load kernel in 0x80000
			while (size--)
				*kernel++ = uart_getc();

			uart_puts("kernel loaded!\r\n");

			void (*kernel_entry)(void) = (void (*)(void))0x80000;
			kernel_entry();

			return;
		} else {
			uart_puts("ERROR: Unrecognized command!\r\n");
		}
	}

	// echo everything back
	while (1) {
		uart_send(uart_getc());
	}
}
