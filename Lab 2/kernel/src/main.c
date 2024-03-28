#include "type.h"
#include "uart.h"
#include "shell.h"
#include "dtb.h"
#include "initrd.h"
#include "string.h"


static void
_kernel_init(uint64_t x0, void (*ptr)(uint64_t))
{
	uart_init();
	uart_line("================================");
	uart_line("Hello Lab 2! It's kernel!");
	uart_line("================================");
	uart_str("main() = 0x"); mini_uart_hex((uint32_t) ptr); mini_uart_endl();
    byteptr_t dtb_ptr = (byteptr_t) x0;
	dtb_set_ptr(dtb_ptr);
	uart_str("DTB    = 0x"); mini_uart_hex((uint32_t) dtb_ptr); mini_uart_endl();
	fdt_traverse(fdt_find_initrd_addr);
	byteptr_t _initrd_ptr = initrd_get_ptr();
	uart_str("initrd = 0x"); mini_uart_hex((uint32_t) _initrd_ptr); mini_uart_endl();
	uart_line("================================");
}


void 
main(uint64_t x0) 
{
	_kernel_init(x0, &main);
	shell();
}
