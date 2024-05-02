#include "../header/mini_uart.h"
#include "../header/shell.h"
#include "../header/devicetree.h"

extern void *_dtb_ptr;

void kernel_main(void)
{
    uart_send_string("Hello, world!\n");

    fdt_traverse(get_initramfs_addr, _dtb_ptr);
    
    shell();
}