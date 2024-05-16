#include "uart.h"
#include "shell.h"
#include "memory.h"
#include "string.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"
#include "schedule.h"

char* dtb_ptr;
// extern void *dtb_ptr;

void main(char* arg){
    // int i = 999999999;
    // while(i--){}
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_cb_init);

    // initial
    uart_init();
    uart_sendline("Loaded success\n");
    
    task_list_init();
    timer_list_init();

    uart_interrupt_enable();
    enable_irq();
    core_timer_enable();
    
    uart_sendline("Press enter...\n");
    cmd_read(input_buffer);
    init_memory();
    init_thread_sched();
    
    while(1){
        cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_sendline("=> ");
        cmd_read(input_buffer);
        //disable_irq();
        cmd_exec(input_buffer);
        //enable_irq();
    }

}