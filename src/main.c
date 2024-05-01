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

// get the end of bss segment from linker
extern unsigned char _end;

void main()
{
    shell_init();
    fdt_init();
    timer_init();
    tasklet_init();
    mm_init();
    sched_init();

    move_to_user_mode();
    while (1)
        asm volatile("nop");
}
