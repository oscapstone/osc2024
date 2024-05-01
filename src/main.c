#include "uart.h"
#include "shell.h"
#include "dtb.h"
#include "exception.h"
#include "sched.h"
#include "timer.h"
#include "interrupt.h"
#include "mm.h"
#include "demo.h"
#include "exec.h"

#define CMD_LEN 128

// get the end of bss segment from linker
extern unsigned char _end;

#define CMD_LEN 128

void main()
{
    shell_init();
    fdt_init();
    timer_init();
    tasklet_init();
    mm_init();
    sched_init();

    // move_to_user_mode();
    while(1) {
        uart_puts("# ");
        char cmd[CMD_LEN];
        shell_input(cmd);
        shell_controller(cmd);
    }
    // if (fork() == 0) {
    //     while(1) {
    //         uart_puts("# ");
    //         char cmd[CMD_LEN];
    //         shell_input(cmd);
    //         shell_controller(cmd);
    //     }
    // } else {
    //     while (1)
    //         asm volatile("nop");
    // }
}
