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

    if (!mem_init((uintptr_t)x0))
        goto inf_loop;

    // int cond;
    // /* get dtb addr */
    // cond = fdt_init((uintptr_t)x0);
    // if (cond)
    //     goto inf_loop;

    uintptr_t dtb_start = get_dtb_start();
    uart_send_string("DTB addr: 0x");
    uart_send_hex(dtb_start >> 32);
    uart_send_hex(dtb_start);
    uart_send_string("\n");

    /* get cpio addr */
    // cond = cpio_init();
    // if (cond)
    //     goto inf_loop;

    uintptr_t cpio_start = get_cpio_start();
    uart_send_string("CPIO addr: 0x");
    uart_send_hex(cpio_start >> 32);
    uart_send_hex(cpio_start);
    uart_send_string("\n");

    /* get root node (for the size of reg propery of the children node)*/
    // cond = fdt_traverse(fdt_find_root_node);
    // if (cond)
    //    goto inf_loop;

    /* get usable memory */
    // cond = fdt_traverse(fdt_find_memory_node);
    // if (cond)
    //     goto inf_loop;

    buddy_init();

    shell();

inf_loop:
    while (1)
        ;
}
