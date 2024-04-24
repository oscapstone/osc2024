#include "exception.h"
#include "fdt.h"
#include "initramfs.h"
#include "ring_buffer.h"
#include "uart.h"
#include "shell.h"

void main()
{
    init_exception_vectors();
    init_interrupt();
    uart_init();
    *UART_IRQs1 |= (1 << 29);   // TODO: refactor
    uart_init_buffer();

    fdt_traverse(initramfs_callback);

    uart_async_puts("hello async!!\n");

    uart_puts("\nWelcome to kernel!\n");
    shell_start();
}