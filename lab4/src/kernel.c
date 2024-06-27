#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mem_utils.h"
#include "../include/dtb.h"
#include "../include/exception.h"
#include <limits.h>

extern char *cpio_addr;
extern char *cpio_addr_end;
extern char *dtb_end;
char *dtb_start;

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

	/* traverse devicetree, and get start and end of cpio and devie tree */
	uint64_t dtb_addr = x0;
	dtb_start = (char *)dtb_addr;
	fdt_traverse(get_cpio_addr, dtb_addr);
	fdt_traverse(get_cpio_end, dtb_addr);
	set_dtb_end(dtb_addr);
	printf("The address of cpio_start: %8x\n", cpio_addr);
	printf("The address of cpio_end: %8x\n", cpio_addr_end);
	printf("The address of dtb_start: %8x\n", dtb_addr);
	printf("The address of dtb_end: %8x\n", dtb_end);

	/* test of printf function */
	// printf( "Hello %s!\n"
    //         "This is character '%c', a hex number: %x and in decimal: %d\n"
    //         "Padding test: '%8x', '%8d'\n",
    //         "World", 'A', 32767, 32767, 0x7FFF, -123);
	
	printf("CHAR_MIN: %d\n", CHAR_MIN);       // checking the type of char in ARM architecture
	enable_interrupt();
	buddy_system_init();
	dynamic_allocator_init();
	shell();
}