#include "uart.h"
#include "io.h"
#include "shell.h"
#include "devtree.h"
#include "exception.h"
#include "timer.h"

int main(){

    uart_init();
    fdt_traverse(initramfs_callback);
    enable_interrupt();
    init_time_queue();
    init_task_queue();
    // core_timer_enable();
    
    print_str("\nLogin Shell");

    while(1){
        shell();
    }

    return 0;
}