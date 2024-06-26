#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mem_utils.h"
#include "../include/dtb.h"
#include "../include/exception.h"
#include "../include/sched.h"
#include "../include/fork.h"
#include "../include/timer.h"
#include "../include/thread.h"
#include <limits.h>

extern char *cpio_addr;
extern char *cpio_addr_end;
extern char *dtb_end;
char *dtb_start;

void process(char *array)
{
	while (1){
		for (int i = 0; i < 5; i++){
			uart_send(array[i]);
			delay(10000);
		}
	}
}

void kernel_main(uint64_t x0)
{
	uart_init();

	uint64_t el = 0;
	asm volatile ("mrs %0, CurrentEL":"=r"(el));
	printf("Current exception level: %d\n", el >> 2);

	/* traverse devicetree, and get start and end of cpio and devie tree */
	uint64_t dtb_addr = x0;
	dtb_start = (char *)dtb_addr;
	fdt_traverse(get_cpio_addr, dtb_addr);
	fdt_traverse(get_cpio_end, dtb_addr);
	set_dtb_end(dtb_addr);
	printf("The address of cpio_start: %8x\n", cpio_addr);
	printf("The address of cpio_end: %8x\n", cpio_addr_end);
	printf("The address of dtb_start: %8x\n", dtb_addr);
	printf("The address of dtb_end: %8x\n", dtb_end);

	/* scheduler start */
	buddy_system_init();
	dynamic_allocator_init();

	int num = 10;
	for (int i = 0; i < num; i++) { 
		copy_process((unsigned long)&foo, (unsigned long)0);    // 0 is unused
	}
	
	enable_interrupt();
	
	/* add initial timer */
	int duration = 2;
	char *msg = NULL;
	uint32_t is_periodic = 1;
	add_timeout_event(msg, duration, is_periodic);

	idle();
}