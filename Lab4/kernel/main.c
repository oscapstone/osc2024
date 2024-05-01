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
    init_buffer();
    init_time_queue();
    init_task_queue();

    enable_interrupt();
    enable_uart_interrupt();
    enable_core_timer();
    
    init_mem();

    async_uart_puts("\nLogin Shell");

    while(1){
        shell();
    }

    return 0;
}