#include "../include/uart.h"
#include "../include/shell.h"
#include "../include/dtb.h"
#include "../include/exception.h"
#include "../include/my_stdlib.h"
#include "../include/my_stdint.h"
#include "../include/allocator.h"

extern char *cpio_addr;
extern char *dtb_addr;

int main(uint64_t x0){
    // set up serial console
    uart_init();

    uint64_t el = 0;
	//asm volatile ("mrs %0, CurrentEL":"=r"(el));
    el = read_register(CurrentEL);
	uart_puts("Current exception level: ");
	uart_hex(el>>2);     // first two bits are preserved bit.
	uart_puts("\n");


    dtb_addr = (void *) x0;
    fdt_traverse(dtb_addr, initramfs_callback);

    startup_allocation();

    enable_interrupt();
    
    initialize_async_buffers();
    // start shell
    shell_start();



    return 0;
}
