#include "../peripherals/mini_uart.h"
#include "bootloader.h"
#include "../peripherals/utils.h"

void bootloader_main(unsigned long address) {
    // Location to load the kernel.
    char* kernel = (char *)0x80000;

    // Store the size of the kernel image.
    unsigned int size = 0;

    // Setup mini uart.
    uart_init();
    uart_send_string("\r\nBooting...\r\n");
    uart_send_string("Kernel image size: ");
    uart_send_string("0x");
    size = get_kernel_size();
    uart_send_uint(size);
    uart_send_string(" bytes\r\n");

    while (size--) {
        // uart_send_string("Loading Kernel\r\n");
        *kernel++ = uart_recv();
    }

    uart_send_string("Kernel loaded.\r\n");

    // Give uart enough time to send "Kernel loaded." message.
    delay(5000);

    asm volatile(
        "mov x0, x10;"
        "mov x1, x11;"
        "mov x2, x12;"
        "mov x3, x13;"
        "mov x30, 0x80000;"
        "ret;"
    );

}

unsigned int get_kernel_size(void) {
    unsigned int size = 0;

    // The uart transfers 4 bytes at a time, while the uart_recv() reads 1 byte every time.
    for (int i = 0; i < 4; i++) {
        char c = uart_recv();
        size |= ((unsigned int)c & 0xFF) << (i * 8);
    }

    return size;
}