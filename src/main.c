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
    shell_init(); // initialize the uart.
    fdt_init(); // initialize the device tree blob and cpio base address.
    timer_init(); // initialize timer pool.
    tasklet_init(); // initialize tasklet related structures.
    mm_init(); // Initialize the memory management: memblock, buddy, slab.

    /* Switch to el0 with interrupt enabled. */
    move_to_user_mode();
    while(1) {
        uart_puts("# ");
        char cmd[CMD_LEN];
        shell_input(cmd);
        shell_controller(cmd);
    }
}
