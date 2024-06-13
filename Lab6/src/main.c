#include "uart1.h"
#include "shell.h"
#include "dtb.h"

#include "memory.h"
#include "exception.h"
#include "irqtask.h"
#include "timer.h"
#include "sched.h"

void main(char* arg){
    // int i = 999999999;
    // while(i--){}
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = PHYS_TO_VIRT(arg);
    traverse_device_tree(dtb_ptr, dtb_cb_init);
    
    // initial
    init_memory();
    uart_init();
    
    task_list_init();
    timer_list_init();

    init_thread_sched();
    uart_interrupt_enable();
    enable_irq();
    core_timer_enable();
    
    uart_sendline("Press enter...\n");
    cmd_read(input_buffer);
    
    
    while(1){
        cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_sendline("=> ");
        cmd_read(input_buffer);
        cmd_exec(input_buffer);
    }

}