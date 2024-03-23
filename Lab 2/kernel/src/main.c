#include "type.h"
#include "mini_uart.h"
#include "shell.h"
#include "dtb.h"
#include "initrd.h"
#include "string.h"


static void
kernel_init(uint64_t x0, void (*ptr)(uint64_t))
{
	mini_uart_init();
	
	mini_uart_putln("\nHello Lab 2! It's kernel!");
	mini_uart_puts("main() = 0x"); mini_uart_hex((uint32_t) ptr); mini_uart_endl();

    byteptr_t _dtb_ptr = (byteptr_t) x0;
	dtb_set_ptr(_dtb_ptr);
	mini_uart_puts("DTB    = 0x"); mini_uart_hex((uint32_t) _dtb_ptr); mini_uart_endl();

	fdt_traverse(fdt_find_initrd_addr);
	byteptr_t _initrd_ptr = initrd_get_ptr();
	mini_uart_puts("initrd = 0x"); mini_uart_hex((uint32_t) _initrd_ptr); mini_uart_endl();
}


void 
main(uint64_t x0) 
{
	kernel_init(x0, &main);
	shell();
}
