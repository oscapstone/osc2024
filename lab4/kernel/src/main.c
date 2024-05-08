#include "uart1.h"
#include "shell.h"
#include "memory.h"
#include "u_string.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"

char* dtb_ptr;
extern char* _end;

void main(char* arg){
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs); // get initramfs location from dtb

    uart_init();
    //uart_2hex((unsigned int)&_end);
    irqtask_list_init();
    timer_list_init();

    uart_interrupt_enable();
    el1_interrupt_enable();  // enable interrupt in EL1 -> EL1
    core_timer_enable();

    cli_cmd_read(input_buffer); // Wait for input, Windows cannot attach to SERIAL from two processes.

    init_allocator();

    cli_print_banner();
    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
