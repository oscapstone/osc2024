#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "u_string.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"

char* dtb_ptr;

void main(char* arg){
    // char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs); // get initramfs location from dtb

    cli_cmd_init();
    uart_init();
    // uart_sendline("\n");
    irqtask_list_init();
    timer_list_init();

    uart_interrupt_enable();
    uart_flush_FIFO();
    core_timer_enable();


    el1_interrupt_enable();  // enable interrupt in EL1 -> EL1
    cli_print_banner();
    cli_cmd();
}
