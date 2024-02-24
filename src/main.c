#include "uart.h"
#include "power.h"
#include "shell.h"
#include "mbox.h"
#include "stdlib.h"
#include "dtb.h"
#include "initrd.h"
#include "exception.h"
#include "delays.h"
#include "timer.h"

#define CMD_LEN 128

extern void move_to_user_mode(void); // defined in exception_.S
extern core_timer_enable(void); // defined in timer_.S

void main()
{
    shell_init();
    // uart_async_init();
    fdt_init();
    fdt_traverse(initramfs_callback);

    // read the current level from system register. If we were el0 with spsr_el1 == 0, we can't access CurrentEL.
    unsigned long el;
    asm volatile ("mrs %0, CurrentEL\n\t" : "=r" (el));
    uart_puts("Current EL is: ");
    uart_hex((el >> 2) & 3);
    uart_puts("\n");

    timer_init();
    core_timer_enable();

    /* Switch to el0 before running shell */
    move_to_user_mode();
    while(1) {
        uart_puts("# ");
        char cmd[CMD_LEN];
        shell_input(cmd);
        shell_controller(cmd);
    }
}
