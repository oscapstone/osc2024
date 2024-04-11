#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "utils.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"

extern char* dtb_ptr;

void main(char* arg){
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs);

    uart_init();
    // uart_sendline("\n");
    // uart_puts("loading dtb from: 0x%x\n", arg);
    uart_interrupt_enable();
    // el1_interrupt_enable();  // enable interrupt in EL1 -> EL1

    core_timer_enable();
    timer_list_init();
    cli_print_banner();

    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("Key in command: ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
