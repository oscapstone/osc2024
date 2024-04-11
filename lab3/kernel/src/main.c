#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "dtb.h"
#include "timer.h"
#include "exception.h"

extern char *dtb_ptr;

void main(char *arg)
{
    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs);

    uart_init();
    irqtask_list_init();
    timer_list_init();

    uart_interrupt_enable();
    uart_flush_FIFO();
    core_timer_enable();

    el1_interrupt_enable(); // enable interrupt in EL1 -> EL1

    start_shell();
}