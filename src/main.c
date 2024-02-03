#include "uart.h"
#include "power.h"
#include "shell.h"
#include "mbox.h"
#include "stdlib.h"
#include "dtb.h"
#include "initrd.h"
#include "exception.h"
#include "sched.h"

#define CMD_LEN 128

extern void move_to_user_mode(void); // defined in exception_.S
extern core_timer_enable(void); // defined in timer_.S

void main()
{
    shell_init();
    // uart_async_init();
    fdt_init();
    fdt_traverse(initramfs_callback);

    timer_init();
    core_timer_enable();
    print_current_el(); // read the current level from system register.

    task_init();

    disable_interrupt();

    sched_init(); // start schedule

    /* Switch to el0 before running shell. Unnessasary in lab 4*/
    // move_to_user_mode();
    while(1) {
        uart_puts("# ");
        char cmd[CMD_LEN];
        shell_input(cmd);
        shell_controller(cmd);
    }
}
