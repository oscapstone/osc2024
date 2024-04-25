#include "uart.h"
#include "shell.h"
#include "dtb.h"
#include "initrd.h"
#include "exception.h"
#include "sched.h"
#include "timer.h"
#include "interrupt.h"
#include "mm.h"
#include "demo.h"

#define CMD_LEN 128

// get the end of bss segment from linker
extern unsigned char _end;

#define CMD_LEN 128

void main()
{
    shell_init();
    fdt_init();
    fdt_traverse(initramfs_callback);
    timer_init(); // initialize timer pool
    tasklet_init(); // initialize tasklet related structure

    print_current_el(); // read the current level from system register.

    // disable_interrupt(); // this is necessary in lab 3 basic 3: asynchronous uart

    mm_init(); // Initialize the memory management: memblock, buddy, slab.

    demo_memory_allocator();

    /* Switch to el0 with interrupt enabled. */
    // move_to_user_mode();
    while(1) {
        // uart_puts("# ");
        // char cmd[CMD_LEN];
        // shell_input(cmd);
        // shell_controller(cmd);
    }
}
