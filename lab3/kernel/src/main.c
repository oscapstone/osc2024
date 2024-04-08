#include "uart.h"
#include "mailbox.h"
#include "shell.h"
#include "cpio.h"
#include "allocator.h"
#include "dtb_parser.h"
#include "exception.h"
#include "timer.h"

int main()
{
	uart_init();
	fdt_traverse(initramfs_callback);
	build_file_arr();
	timer_heap_init();
	uart_puts("\n");
	enable_aux_interrupt();
	enable_interrupt();
	simple_shell();

	return 0;
}
