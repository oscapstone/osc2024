#include "uart1.h"
#include "shell.h"
#include "memory.h"
#include "u_string.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"

char* dtb_ptr = 0;

void main(char* arg){
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs); // get initramfs location from dtb

    cli_cmd_init();
    uart_init();
    irqtask_list_init();
    timer_list_init();

    uart_interrupt_enable();
    uart_flush_FIFO();

    core_timer_enable();
    el1_interrupt_enable();  // enable interrupt in EL1 -> EL1
    // while(1){
    //     for (int i = 0; i < 100000; i++) asm volatile("nop");
    //     uart_sendline("Hello, World!\n");
    // }
    //     uart_sendline("aaaaa\n");
    //     uart_send('c');

    uart_sendline("please press enter to init allocator\n");
    cli_cmd_read(input_buffer); // Wait for input, Windows cannot atch to SERIAL from two processes.
    init_allocator();


    cli_print_banner();
    cli_cmd();

}
