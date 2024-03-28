#include "../peripherals/mini_uart.h"
#include "shell.h"
#include "initramdisk.h"
#include "device_tree.h"

void kernel_main(uintptr_t dtb_address)
{
	uart_init();
    init_heap();
    //fdt_traverse(print_tree, dtb_address);
    fdt_traverse(get_initrd_address, dtb_address);
    shell();
}
