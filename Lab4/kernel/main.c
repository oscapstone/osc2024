#include "uart.h"
#include "io.h"
#include "shell.h"
#include "devtree.h"
#include "exception.h"
#include "timer.h"
#include "mini_uart.h"
#include "mem.h"

int main(){

    uart_init();
    fdt_traverse(initramfs_callback);
    enable_interrupt();
    enable_uart_interrupt();
    enable_core_timer();
    init_time_queue();
    init_task_queue();
    init_buffer();
    init_mem();
    
    print_str("\nLogin Shell");

    while(1){
        shell();
    }

    return 0;
}