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
#include "../include/sys.h"
#include "../include/mailbox.h"
#include <limits.h>

extern char *cpio_addr;
extern char *cpio_addr_end;
extern char *dtb_end;
char *dtb_start;

extern volatile unsigned int  __attribute__((aligned(16))) mbox_buf[8];

// void user_process1(char *array)
// {
// 	char buf[2] = {0};
// 	while (1) {
// 		for (int i = 0; i < 5; i++) {
// 			buf[0] = array[i];
// 			call_sys_write(buf);
// 			delay(100000);
// 		}
// 	}
// }

void user_process()
{
	// char buf[30] = {0};
	// printf("User process started\n");
	// int pid = getpid();
	// printf("user->pid: %d\n", pid);
	// uint32_t read_char = uart_read(buf, 30);
	// printf("read %d characters\n", read_char);
	// uint32_t write_char = uart_write(buf, 30);
	// printf("write %d characters\n", write_char);
	// int num = exec(NULL, NULL);
	// printf("exec: %d\n", num);
	// int pid = fork();
	// printf("fork pid: %d\n", pid);
	// get_board_revision();
	// sys_kill(pid);
	exit_process();
}

void kernel_process()
{
	printf("Kernel process started.\n");
	int err = move_to_user_mode((unsigned long)&user_process);         // set trap frame
	if (err < 0)
		printf("Error while moving process to user mode.\n");
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
	enable_interrupt();

	int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, page_frame_allocate(4));
	if (res < 0) {
		printf("error while starting kernel process");
		return;
	}

	while (1) {
		schedule();
	}

	// int num = 10;
	// for (int i = 0; i < num; i++) { 
	// 	copy_process((unsigned long)&foo, (unsigned long)0);    // 0 is unused
	// }
	
	/* add initial timer */
	int duration = 2;
	char *msg = NULL;
	uint32_t is_periodic = 1;
	add_timeout_event(msg, duration, is_periodic);

	// idle();
}