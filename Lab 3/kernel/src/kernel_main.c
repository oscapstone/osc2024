#include "kernel.h"
#include "uart.h"
#include "irq.h"
#include "shell.h"
#include "dtb.h"
#include "initrd.h"
#include "string.h"
#include "core_timer.h"
#include "exception.h"


static void
_kernel_init(uint64_t x0, void (*ptr)(uint64_t))
{
    byteptr_t dtb_ptr = (byteptr_t) x0;
	dtb_set_ptr(dtb_ptr);
	fdt_traverse(fdt_find_initrd_addr);
	byteptr_t initrd_ptr = initrd_get_ptr();
	uint32_t current_el = current_exception_level();

    exception_l1_enable();
	uart_init();

	uart_line("+------------------------------+");
	uart_line("|       Lab 3 - Kernel         |");
	uart_line("+------------------------------+");
	uart_str ("|      main() = 0x"); mini_uart_hex((uint32_t) ptr); uart_line("     |");
	uart_str ("|      DTB    = 0x"); mini_uart_hex((uint32_t) dtb_ptr); uart_line("     |");
	uart_str ("|      initrd = 0x"); mini_uart_hex((uint32_t) initrd_ptr); uart_line("     |");
	uart_str ("|   CurrentEL = 0x"); mini_uart_hex((uint32_t) current_el); uart_line("     |");
	uart_line("+------------------------------+");
}


void 
kernel_main(uint64_t x0) 
{
	_kernel_init(x0, &kernel_main);
	shell();
}
