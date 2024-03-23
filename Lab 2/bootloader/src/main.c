#include "type.h"
#include "mini_uart.h"
#include "shell.h"


byteptr_t dtb_ptr;

void
main(uint64_t x0) 
{
	void (*main_ptr)(uint64_t) = &main;

	mini_uart_init();
	mini_uart_putln("Hello Lab 2! It's bootloader!");
	mini_uart_puts("main() = 0x"); mini_uart_hex((uint32_t) main_ptr); mini_uart_putln("");

	dtb_ptr = (byteptr_t) x0;
	mini_uart_puts("DTB    = 0x"); mini_uart_hex((uint32_t) dtb_ptr); mini_uart_putln("");

	shell();
}