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

    dtb_init(PHYS_TO_VIRT(arg));

    uart_init();
    uart_interrupt_enable();
    uart_flush_FIFO();
    core_timer_enable();

    DEBUG("memory init\r\n");
    memory_init();
    DEBUG("irqtask_list_init\r\n");
    irqtask_list_init();
    DEBUG("timer_list_init\r\n");
    timer_list_init();
    DEBUG("sched_init\r\n");
    init_thread_sched();

    load_context(&curr_thread->context); // jump to idle thread and unlock interrupt
}