#include "uart.h"
#include "mailbox.h"
#include "shell.h"
#include "cpio.h"
#include "allocator.h"
#include "dtb_parser.h"
#include "exception.h"
#include "task.h"
#include "timer.h"
#include "allocator.h"

int main()
{
	uart_init();
	fdt_traverse(initramfs_callback);
	build_file_arr();
	task_heap_init();
	timer_heap_init();
	allocator_init();
	/*uart_puts("\n");
	enable_aux_interrupt();
	enable_interrupt();
	asm volatile(
        "mov x0, 1\n"
        "msr cntp_ctl_el0, x0\n"); // enable core0 timer
	simple_shell();*/

	uart_puts("\n");
	print_free_area();
	uart_puts("-------------------------\n");

	char* ptr = malloc(5000);
	uart_puts("\n");
	print_free_area();
	uart_puts("-------------------------\n");

	char* ptr2 = malloc(5000);
	uart_puts("\n");
	print_free_area();
	uart_puts("-------------------------\n");

	free(ptr);
	uart_puts("\n");
	print_free_area();
	uart_puts("-------------------------\n");

	free(ptr2);
	uart_puts("\n");
	print_free_area();
	uart_puts("-------------------------\n");


	return 0;
}
