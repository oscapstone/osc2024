#include "uart.h"
#include "power.h"
#include "shell.h"
#include "mbox.h"
#include "stdlib.h"
#include "dtb.h"
#include "initrd.h"
#include "sd.h"
#include "exception.h"
#include "sched.h"
#include "timer.h"

#define CMD_LEN 128

// get the end of bss segment from linker
extern unsigned char _end;

#define CMD_LEN 128

extern void move_to_user_mode(void); // defined in exception_.S
extern void core_timer_enable(void); // defined in timer_.S

void main()
{
    shell_init();

    // initialize EMMC and detect SD card type
    // if(sd_init()==SD_OK) {
    //     // read the master boot record after our bss segment
    //     if(sd_readblock(0,&_end,1)) {
    //         // dump it to serial console
    //         uart_dump(&_end);
    //     }
    // }

    get_board_revision();
    get_memory_info();

    // uart_async_init();
    fdt_init();
    fdt_traverse(initramfs_callback);

    timer_init();
    // core_timer_enable(); // User have to use `timer_on` to enable timer before `set_timeout`.

    print_current_el(); // read the current level from system register.

    task_init();

    disable_interrupt();

    sched_init(); // start schedule

    /* Switch to el0 before running shell. Unnessasary in lab 4*/
    // move_to_user_mode();
    while(1) {
        // uart_puts("# ");
        // char cmd[CMD_LEN];
        // shell_input(cmd);
        // shell_controller(cmd);
    }
}
