#include "uart.h"
#include "io.h"
#include "shell.h"
#include "devtree.h"
#include "exception.h"
#include "timer.h"
#include "mini_uart.h"
#include "mem.h"
#include "sched.h"

int main(){

    uart_init();
    fdt_traverse(initramfs_callback);

    init_mem();
    init_scheduler();

    init_buffer();
    init_time_queue();
    init_task_queue();

    enable_interrupt();
    enable_uart_interrupt();
    enable_core_timer();
    set_kernel_timer();
    
    async_uart_puts("\nLogin Shell");

    // int N = 3;
    // for(int i = 0; i < N; ++i) { // N should > 2
    //     thread_create(foo);
    // }
    
    // Start Scheduler
    schedule_timer();

    idle();

    return 0;
}