#include "mini_uart.h"
#include "shell.h"
#include "alloc.h"
#include "fdt.h"
#include "initrd.h"
#include "c_utils.h"
#include "utils.h"

extern char *dtb_base;

void main(char* base) {
	dtb_base = base;
	uart_init();
	alloc_init();
	simple_malloc(8);
	uart_send_string("DTB base address: ");
	uart_hex(dtb_base);
	uart_send_string("\n");
	fdt_traverse(initrd_callback);
	shell();
}
