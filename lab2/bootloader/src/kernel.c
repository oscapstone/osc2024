#include "mini_uart.h"
#include "bootloader.h"

void kernel_main()
{
    uart_init();
    // int t=500;
    
    // while(t--)
    // {
	// asm volatile("nop");
    // }
    
    uart_send_string("Start Bootloading......\n\r");
    load_img();
}
