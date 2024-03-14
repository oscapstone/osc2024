#include "mini_uart.h"
// #include "shell.h"

void kernel_main(void)
{
	uart_init();

	uart_send_string("\x1b[31mWelcome to Raspberry Pi!\x1b[0m\r\n");
    uart_send_string("      .~~.   .~~.       \r\n");
    uart_send_string("     '. \\ ' ' / .'      \r\n");
    uart_send_string("      .~ .~~~..~.       \r\n");
    uart_send_string("     : .~.'~'.~. :      \r\n");
    uart_send_string("    ~ (   ) (   ) ~     \r\n");
    uart_send_string("   ( : '~'.~.'~' : )    \r\n");
    uart_send_string("    ~ .~ (   ) ~. ~     \r\n");
    uart_send_string("     (  : '~' :  )      \r\n");
    uart_send_string("      '~ .~~~. ~'       \r\n");
    uart_send_string("          '~'           \r\n");
	uart_send_string("\r\n");
	// uart_send_string("Hello, world!\r\n");

	while (1) {
		// uart_send(uart_recv());
		char buf[100];
		// uart_recv_string(s);
		// uart_send_string(s);
		uart_recv_string(buf);
		parse_command(buf);
        // uart_send_string("\n");
	}
}