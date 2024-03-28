#include "stdint.h"

extern char _kernel_start[];  // 0x80000
char* _dtb;

void load_kernel() {
    uint32_t size;
    uart_read(&size, 4);  // read 4 byte magic code

    uart_write_string("[+] Loading kernel.img from UART\r\n");  //

    // read `kernel.img` size
    uart_read(&size, 4);
    uart_write_string("[+] Get kernel.img size: 0x");
    uart_puth(size);
    uart_write_string("\r\n");

    // read `kernel.img`
    uart_read(_kernel_start, size);

    // finish!
    uart_write_string("[+] Load kernel.img successfully!\r\n");
}


void bootloader_main(char* x0) {
    _dtb = x0;  // store x0
    uart_init();
    load_kernel();
    ((void (*)())_kernel_start)();
    ((void (*)(char*))_kernel_start)(_dtb);
}