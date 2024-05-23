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

extern thread_t *curr_thread;
extern thread_t *threads[];

void main(char *arg)
{
    kernel_lock_interrupt();

    dtb_init(arg);

    uart_init();
    uart_interrupt_enable();
    uart_flush_FIFO();

    DEBUG("memory init\r\n");
    memory_init();
    DEBUG("irqtask_list_init\r\n");
    irqtask_list_init();
    DEBUG("timer_list_init\r\n");
    timer_list_init();
    DEBUG("sched_init\r\n");
    init_thread_sched();
    // while (1)
    //     ;

    // el1_interrupt_enable(); // enable interrupt in EL1 -> EL1
    char *str;
    // start_shell();
    DEBUG("thread test\r\n");
    // DEBUG_BLOCK({
    //     for (int i = 0; i < 5; ++i)
    //     { // N should > 2
    //         str = kmalloc(6);
    //         sprintf(str, "foo_%d", i);
    //         thread_create(foo, str);
    //         DEBUG("create foo_%d\r\n", i);
    //     }
    // });
    str = kmalloc(7);
    sprintf(str, "kshell");
    thread_create(start_shell, str);
    threads[2]->datasize = 0x100000;
    schedule_timer();
    core_timer_enable();
    kernel_unlock_interrupt();
    schedule();
}