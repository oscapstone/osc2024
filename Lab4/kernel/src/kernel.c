#include "cpio.h"
#include "dtb.h"
#include "int.h"
#include "irq.h"
#include "logo.h"
#include "memory.h"
#include "mini_uart.h"
#include "shell.h"
#include "string.h"
#include "timer.h"
#include "utils.h"

void kernel_main(uintptr_t x0)
{
    uart_init();
    uart_send_string("Welcome to Raspberry Pi 3B+\n");
    send_logo();

    uart_send_string("Current exception level: ");
    uart_send_dec(get_el());
    uart_send_string("\n");

    irq_vector_init();

    enable_irq();

    /* get dtb addr */
    set_dtb_ptr((uintptr_t)x0);

    unsigned long dtb_ptr = get_dtb_ptr();
    uart_send_string("DTB addr: 0x");
    uart_send_hex(dtb_ptr >> 32);
    uart_send_hex(dtb_ptr);
    uart_send_string("\n");

    /* get cpio addr */
    fdt_traverse(fdt_find_cpio_ptr);

    unsigned long cpio_ptr = get_cpio_ptr();
    uart_send_string("CPIO addr: 0x");
    uart_send_hex(cpio_ptr >> 32);
    uart_send_hex(cpio_ptr);
    uart_send_string("\n");

    /* get root node (for the size of reg propery of the children node)*/
    fdt_traverse(fdt_find_root_node);

    /* get usable memory */
    fdt_traverse(fdt_find_memory_node);
    uintptr_t usable_mem_start = get_start_addr();
    uintptr_t usable_mem_length = get_length();

    shell();

    while (1)
        ;
}
