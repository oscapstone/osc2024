#include "uart1.h"
#include "shell.h"
#include "memory.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"
#include "sched.h"
#include "uart1.h"
#include "irqtask.h"
#include "vfs.h"
//#include "bcm2837/rpi_mmu.h"

extern list_head_t *run_queue;
extern char *dtb_ptr;
int Set_dtb = 0;

void main(char *arg)
{
    // uart_sendlinek("in main\n");
    dtb_ptr = (char *)PHYS_TO_KERNEL_VIRT(arg);
    if (!Set_dtb)
    {
        traverse_device_tree(dtb_ptr, dtb_callback_initramfs);
        Set_dtb = 1;
    }
    uart_init();
    allocator_init();
    rootfs_init();
    irqtask_list_init();
    timer_list_init();
    thread_sched_init();
    //uart_sendlinek("in main\n");

    core_timer_enable();
    uart_interrupt_enable();
    uart_flush_FIFO();

    el1_interrupt_enable(); // enable interrupt in EL1 -> EL1

    //start_shell();
    // uart_sendlinek("RQ size :  %d \n",list_size(run_queue));
    
    //schedule();
    load_context(&((((thread_t*)run_queue->next))->context));
}
