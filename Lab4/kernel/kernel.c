#include "../peripherals/mini_uart.h"
#include "shell.h"
#include "initramdisk.h"
#include "device_tree.h"
#include "irq.h"
#include "timer.h"
#include "task.h"
#include "memory_alloc.h"


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


    enable_el1_interrupt();

    uart_send_string("Time after booting: ");
    uart_send_uint(get_current_time());
    uart_send_string(" seconds\r\n");

    irq_vector_init();
    core_timer_enable();
    init_heap();
    init_heap2();
    init_frame_freelist();
    init_task_queue();
    init_memory_pool();
    fdt_traverse(get_initrd_address, dtb_address);
    if (parse_reserved_memory(dtb_address + myHeader.off_mem_rsvmap) != 0) {
        uart_send_string("Error parsing memory reservation block!\r\n");
    }
    // fdt_traverse(parse_rvmem_child, dtb_address);

    // Check preemption.
    // add_timer(create_loop, NULL, 2);
    // add_timer(exit_loop, NULL, 5);

    // add_timer(delay_loop, NULL, 2);
    // char* message = (char *)simple_malloc(sizeof(char) * 10);
    // strcpy(message, "Timeout\r\n");
    // add_timer(print_timeout_message, (char *)message, 4);

    //while(1);
    //fdt_traverse(print_tree, dtb_address);
    shell();

}
