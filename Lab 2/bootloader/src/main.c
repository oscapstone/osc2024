#include "type.h"
#include "uart.h"
#include "shell.h"


byteptr_t g_dtb_ptr;
uint32_t  g_kernel_loaded;


void
main(uint64_t x0) 
{
	uart_init();
	uart_line("================================");
	uart_line("Hello Lab 2! It's bootloader!");
	uart_line("================================");
	uart_str("main() = 0x"); uart_hex((uint32_t) &main); uart_line("");
	g_dtb_ptr = (byteptr_t) x0;
	uart_str("DTB    = 0x"); uart_hex((uint32_t) g_dtb_ptr); uart_line("");
	uart_line("================================");
	g_kernel_loaded = 0;
	shell();
}