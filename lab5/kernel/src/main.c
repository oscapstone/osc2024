#include "uart1.h"
#include "shell.h"
#include "memory.h"
#include "u_string.h"
#include "dtb.h"
#include "exception.h"
#include "timer.h"
#include "sched.h"

char* dtb_ptr;

void main(char* arg){
    // char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs); // get initramfs location from dtb

    cli_cmd_init();
    uart_init();
    uart_sendline("\n");
    irqtask_list_init();
    timer_list_init();

    uart_interrupt_enable();
    el1_interrupt_enable();  // enable interrupt in EL1 -> EL1
    core_timer_enable();

#if DEBUG
    cli_cmd_read(input_buffer); // Wait for input, Windows cannot attach to SERIAL from two processes.
#endif
    init_allocator();
    init_thread_sched();

    cli_print_banner();
    cli_cmd();
}
