#include "mini_uart.h"
#include "shell.h"
#include "alloc.h"
#include "fdt.h"
#include "initrd.h"
#include "c_utils.h"
#include "exception.h"

#define TEST_SIZE 30

extern char *dtb_base;

void demo_mm() {
	char* page_addr[TEST_SIZE];
	for(int i=0;i<TEST_SIZE;i+=3) {
		page_addr[i] = kmalloc(20);
		page_addr[i+1] = kmalloc(200);
		page_addr[i+2] = kmalloc(PAGE_SIZE);
		// free_list_info();
		print_chunk_info();
	}
	for(int i=0;i<TEST_SIZE;i+=3) {
		kfree(page_addr[i]);
		kfree(page_addr[i+1]);
		kfree(page_addr[i+2]);
		// free_list_info();
		print_chunk_info();
	}
}

void main(char* base) {
	dtb_base = base;
	uart_init();
	uart_send_string("DTB base address: ");
	uart_hex(dtb_base);
	uart_send_string("\n");
	fdt_traverse(initrd_callback);
	alloc_init();
	uart_enable_interrupt();
	demo_mm();
	debug = 0;
	shell();
}
