#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"

extern char *dtb_ptr;
int Set_dtb = 0;

void main(char *arg)
{
    dtb_ptr = arg;
    if (!Set_dtb)
    {
        traverse_device_tree(dtb_ptr, dtb_callback_initramfs);
        Set_dtb = 1;
    }
    uart_init();

    irqtask_list_init();
    timer_list_init();
    uart_interrupt_enable();
    core_timer_enable();
    for (int i = 0; i < 1; i++)
    {
        uart_sendlinek("fk   \n");
    }

    el1_interrupt_enable(); // enable interrupt in EL1 -> EL1
    start_shell();
}