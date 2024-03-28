#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mem_utils.h"
#include "../include/dtb.h"

extern char *cpio_addr;

void kernel_main(uint64_t x0)
{
	uart_init();
	uart_send_string("Reboot finished!!!\r\n");
	uint64_t main_ptr = &kernel_main;
	uart_hex(main_ptr);
	uart_send_string("\r\n");
	// /* test malloc */
	// char *tmp = malloc(4);
	// if (!tmp) {
	// 	uart_send_string("HEAP overflow!!!\r\n");
	// 	return;
	// }
	// tmp[0] = '1';
	// tmp[1] = '2';
	// tmp[2] = '3';
	// tmp[3] = '\0';
	// uart_send_string(tmp);
	// uart_send_string("\r\n");
	
	// /* test strlen */
	// uart_send_string("The length of tmp is: ");
	// uart_hex(my_strlen(tmp));
	// uart_send_string("\r\n");

	/* traverse devicetree */
	uint64_t dtb_addr = x0;
	fdt_traverse(get_cpio_addr, dtb_addr);
	
	uart_hex((uint64_t)cpio_addr);
	uart_send_string("\r\n");
	
	shell();
}