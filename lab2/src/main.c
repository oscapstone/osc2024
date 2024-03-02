#include "alloc.h"
#include "uart.h"
#include "shell.h"
#include "devtree.h"
#include "initrd.h"

int main()
{
	fdt_traverse(initrd_callback);
	alloc_init();
	uart_init();
	uart_puts("Welcome!\n");
	run_shell();
	return 0;
}