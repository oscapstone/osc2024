#include "uart1.h"
#include "shell.h"
#include "memory.h"
#include "dtb.h"
#include "timer.h"
#include "memory.h"
#include "exception.h"


void main(char *arg)
{
    lock_interrupt();

    dtb_init(arg);

    uart_init();
    uart_interrupt_enable();
    uart_flush_FIFO();

    memory_init();

    irqtask_list_init();
    timer_list_init();
    // while (1)
    //     ;
    core_timer_enable();

    unlock_interrupt();
    // el1_interrupt_enable(); // enable interrupt in EL1 -> EL1

    start_shell();
}