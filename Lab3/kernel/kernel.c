#include "../peripherals/mini_uart.h"
#include "shell.h"
#include "initramdisk.h"
#include "device_tree.h"
#include "irq.h"
#include "timer.h"

void kernel_main(uintptr_t dtb_address)
{
    /*
    asm volatile (
        // CurrentEL register(64-bit) stores the current exception level.
        // bits[3:2]
        // 0b00 -> EL0
        // 0b01 -> EL1
        // 0b10 -> EL2
        // 0b11 -> EL3
        "mrs x0, CurrentEL;"
    );
    */
/*
    // Test asynchronous UART.
    irq_vector_init();
    enable_el1_interrupt();
    enable_uart_interrupt();

    // Infinite loop for testing asynchronous UART
    while(1) {}
*/

    enable_el1_interrupt();
    uart_send_string("Time after booting: ");
    uart_send_uint(get_current_time());
    uart_send_string(" seconds\r\n");
    irq_vector_init();
    core_timer_enable();
    init_heap();
    fdt_traverse(get_initrd_address, dtb_address);
    //fdt_traverse(print_tree, dtb_address);
    shell();

}
