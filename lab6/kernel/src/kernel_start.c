#include <mini_uart.h>
#include <shell.h>
#include "fdt.h"
#include "cpio.h"
#include "alloc.h"
#include "thread.h"
#include "mmu.h"

extern void core_timer_enable();
extern char _code_start[];
extern char _code_end[];

extern thread* get_current();

char* fdt_addr;

void kernel_begin(char* fdt) {
	fdt_addr = pa2va(fdt);
	uart_init();
	long t = 0xffff000000000000;
	uart_printf ("%llx translated to %llx\r\n", t, trans(t));

	uart_printf ("Hello world\r\n");

	fdt_traverse(get_initramfs_addr); // also set _cpio_file
	manage_init();
	
	thread_init();
	get_current() -> code = _code_start;
	get_current() -> code_size = (_code_end - _code_start + 1);
	get_current() -> PGD = 0x1000;

	long x = 0xffff000000000000;
	uart_printf ("0x%llx\r\n", &x);

	shell_begin(fdt);
	return;
}
