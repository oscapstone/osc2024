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
    uart_printf("Welcome to Raspberry Pi 3B+\n");
    send_logo();

    uart_printf("Current exception level: %d\n", get_el());

    irq_vector_init();

    enable_irq();

    if (!mem_init((uintptr_t)x0))
        goto inf_loop;

    buddy_init();

    shell();

inf_loop:
    while (1)
        ;
}
