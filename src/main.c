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
#include "syscall.h"
#include "mm.h"

// get the end of bss segment from linker
extern unsigned char _end;

void main()
{
    shell_init();
    print_current_el();
    fdt_init();
    timer_init();
    tasklet_init();
    mm_init();
    sched_init();


    // do_shell_user();
    // printf("shouldn not be here\n");


    /* sched_init() will make kernel be the task 0, then run shell in user mode. So there should not be returned. */
    while (1);
}
