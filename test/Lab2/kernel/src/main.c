#include "uart.h"
#include "shell.h"
#include "cpio.h"
#include "dtb.h"
#include "utils.h"

void main(void *dtbAddress)
{
    // set up serial console
    uart_init();

    // say hello
    uart_puts("Hello World!\n");
    heapInit();
    uart_hex(dtbAddress);
    uart_puts("\n");
    uart_puts("initrams(init): ");
    uart_hex(initrdGet());
    uart_puts("\n");
    fdt_find_do(dtbAddress, "linux,initrd-start", initrd_fdt_callback);
    uart_puts("initrams(loaded): ");
    uart_hex(initrdGet());
    uart_puts("\n");
    shellStart();
}
