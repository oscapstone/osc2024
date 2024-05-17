#include "uart1.h"
#include "shell.h"
#include "memory.h"
#include "dtb.h"
#include "timer.h"
#include "memory.h"
#include "exception.h"
#include "sched.h"
#include "debug.h"
#include "string.h"

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
    init_thread_sched();
    // while (1)
    //     ;

    // el1_interrupt_enable(); // enable interrupt in EL1 -> EL1
    char *str;
    // start_shell();
    DEBUG("thread test\r\n");
    for (int i = 0; i < 5; ++i)
    { // N should > 2
        str = kmalloc(6);
        sprintf(str, "foo_%d", i);
        thread_create(foo, str);
        DEBUG("create foo_%d\r\n", i);
    }
    str = kmalloc(7);
    sprintf(str, "kshell");
    thread_create(start_shell, str);
    schedule_timer();
    core_timer_enable();
    unlock_interrupt();
}