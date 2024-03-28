#include "mini_uart.h"
#include "shell.h"
#include "devicetree.h"
#include "io.h"
#include "string.h"
#include "type.h"
#include "cpio.h"


int main()
{
    mem_init();
    uart_init();
    
    uint32_t* dtb_addr = 0x50000;
	printf("\nDTB Address (Kernel): ");
	printf_hex(*dtb_addr);
    fdt_traverse(initramfs_callback);

    uart_send_string("\nWelcome to Yuchang's Raspberry Pi 3!\n");
    while(1)
    {
        shell();
    }
    return 0;
}