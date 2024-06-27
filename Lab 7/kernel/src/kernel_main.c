#include "kernel.h"
#include "uart.h"
#include "irq.h"
#include "shell.h"
#include "dtb.h"
#include "initrd.h"
#include "string.h"
#include "exception.h"
#include "frame.h"
#include "memory.h"
#include "sched.h"
#include "thread.h"
#include "core_timer.h"
#include "delay.h"
#include "chunk.h"
#include "vfs.h"


/**
 * Note:
 * 1. Spin tables for multicore boot                                (0x0000 - 0x1000)
 * 2. Kernel image in the physical memory							(0x80000 - )
 * 3. Initramfs														(0x08000000 - 0x08002200)
 * 4. Devicetree (Optional, if you have implement it)				(0x08200000 - 0x08215988)
 * 5. Your simple allocator (startup allocator)						
*/

extern byte_t kernel_start;
extern byte_t kernel_end;

static void
_kernel_init(uint64_t x0, void (*ptr)(uint64_t))
{
	dtb_set_ptr((byteptr_t) x0);
	fdt_traverse(fdt_find_initrd_addr);
	memory_system_init(0x0, 0x3C000000);

	uart_init();

	uart_printf("+------------------------------+\n");
	uart_printf("|       Lab 7 - Kernel         |\n");
	uart_printf("+------------------------------+\n");

	uart_printf("|     main()    = 0x%8x   |\n", ptr);
	uart_printf("|  kernel_start = 0x%8x   |\n", &kernel_start);
	uart_printf("|   kernel_end  = 0x%8x   |\n", &kernel_end);

	uart_printf("|  Initrd_start = 0x%8x   |\n", initrd_get_ptr());
	uart_printf("|   Initrd_end  = 0x%8x   |\n", initrd_get_end());

	uart_printf("|   DTB_start   = 0x%8x   |\n", dtb_get_ptr());
	uart_printf("|    DTB_end    = 0x%8x   |\n", dtb_get_end());

	uart_printf("|   frame preserved = %d      |\n", frame_preserved_count());

	uart_line("+------------------------------+");

    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
    
	vfs_init();
	
    exception_l1_enable();
}


void idle()
{
	while (1) {
		kill_zombies();
		schedule();
	}
}


void 
kernel_main(uint64_t x0) 
{
	_kernel_init(x0, &kernel_main);
	scheduler_init();
	thread_create_shell((uint64_t) shell);
	// initrd_exec("syscall.img");
	idle();
}
