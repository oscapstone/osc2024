#include "alloc.h"
#include "uart.h"
#include "shell.h"
#include "devtree.h"
#include "initrd.h"
#include "irq.h"

int main()
{
	uart_init();
	alloc_init();
	enable_core_timer();
	fdt_traverse(initrd_callback);
	uart_puts("Welcome!\n");
	run_shell();
	return 0;
}