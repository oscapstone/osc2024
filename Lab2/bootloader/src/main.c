#include "../include/uart.h"
#include "../include/stdint.h"

void main()
{
    // set up serial console
    uart_init();
    uart_puts("bootloader is coming~\n");

    uint32_t file_size = 0;
    size_t index = 0;
    // Buffer to store received data
    uint8_t byte;

    while(1) {
        if(uart_recv() == ((uint8_t)77)) break;
    }
    uart_puts("ready for receiving kernel size.\n");
    

    // Receive the file size as a 4-byte integer in little-endian format
    for (int i = 0; i < 4; i++) {
        byte = uart_recv();
        file_size |= ((uint32_t)byte) << (8 * i);
    }

    char file_size_str[32];  // Adjust the buffer size as needed
    itos(file_size, file_size_str);

    uart_puts("kernel size :");
    uart_puts(file_size_str);
    uart_puts(" bytes.\n");

    while(1) {
        uart_puts("ready for receiving kernel.\n");
        if(uart_recv() == ((uint8_t)77)) break;
    }
    

    char *kernel = (char *)0x80000;
    while (index < file_size) {
        // Receive a byte
        byte = uart_recv();
        // Write the byte to memory
        *(kernel + index) = byte;
        // Increment the index
        index++;
    }

    uart_puts("finished loading kernel~\n");


    // restore arguments and jump to the new kernel.
    asm volatile (
        "mov x30, 0x80000;" // Move the immediate value 0x80000 to the link register (x30)
        "ret;"              // Return from subroutine (this will jump to the address in x30)
    );
}