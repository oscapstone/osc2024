#include "mini_uart.h"
#include "shell.h"
#include "string.h"
#include "heap.h"
#include "devicetree.h"
#include "initrd.h"
#include "interrupt.h"
#include "timer.h"

// static char user_input[BUFFER_SIZE];

extern void set_exception_vector_table();

void main(void* dtb_ptr){
    uart_init();

    set_exception_vector_table();

    asm volatile("svc 0");
    fdt_traverse(dtb_ptr, initramfs_callback);

    enable_irq();
    core_timer_enable();
    
    shell_exec();
}