#include "../peripherals/mini_uart.h"
#include "shell.h"
#include "initramdisk.h"
#include "device_tree.h"
#include "irq.h"
#include "timer.h"
#include "task.h"
#include "dynamic_alloc.h"
#include "buddy_system_2.h"
#include "thread.h"
#include "kernel.h"


/*
 *  Use 0x1000_0000 ~ 0x2000_0000 for page frame allocator.
 */
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

    init_kernel();

    enable_el1_interrupt();

    uart_send_string("Time after booting: ");
    uart_send_uint(get_current_time());
    uart_send_string(" seconds\r\n");

    fdt_traverse(get_initrd_address, dtb_address);
    fdt_traverse(get_initrd_end_address, dtb_address);
    if (parse_reserved_memory(dtb_address + myHeader.off_mem_rsvmap) != 0) {
        uart_send_string("Error parsing memory reservation block!\r\n");
    }
    
    shell();
}

void init_kernel(void) {
    irq_vector_init();
    core_timer_enable();
    init_heap();
    init_heap2();
    init_task_queue();
    init_dynamic_alloc();
    init_thread_pool();
    init_scheduler();
}
