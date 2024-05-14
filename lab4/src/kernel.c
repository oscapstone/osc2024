#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mem_utils.h"
#include "../include/dtb.h"
#include "../include/exception.h"

extern char *cpio_addr;

void kernel_main(uint64_t x0)
{
	uart_init();

	uint64_t el = 0;
	asm volatile ("mrs %0, CurrentEL":"=r"(el));
	uart_send_string("Current exception level: ");
	uart_hex(el>>2);     // first two bits are preserved bit.
	uart_send_string("\n");
	// uart_send_string("Reboot finished!!!\r\n");
	// uint64_t main_ptr = &kernel_main;
	// uart_hex(main_ptr);
	// uart_send_string("\r\n");

	/* traverse devicetree */
	uint64_t dtb_addr = x0;
	fdt_traverse(get_cpio_addr, dtb_addr);
	uart_send_string("The address of cpio: ");
	uart_hex((uint64_t)cpio_addr);
	uart_send_string("\r\n");
	
	enable_interrupt();
	shell();
}