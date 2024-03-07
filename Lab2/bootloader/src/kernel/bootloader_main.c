#include "kernel/uart.h"

void bootloader_main(){
    unsigned int kernel_size = 0;
    // a char pointer has the same alignment requirement as a void pointer.
    char *kernel_addr = (char *)0x80000;
    unsigned char recv[4];
    uart_init();
    uart_puts("Hello, world! 312552025\r\n");
    uart_puts("Start bootloading\n");

    while(1){
        if(uart_getc_img() == (unsigned char)'!'){
            uart_puts("Confirmed\n");
            break;
        }
    }
    // since the data is little endian, LSB will at lowest address(first received)
    recv[0] = uart_getc_img();
    kernel_size = recv[0];
    recv[1] = uart_getc_img();
    kernel_size |= (recv[1] << 8);
    recv[2] = uart_getc_img();
    kernel_size |= (recv[2] << 16);
    recv[3] = uart_getc_img();
    kernel_size |= (recv[3] << 24);

    /*while(1){
        uart_putc(uart_getc());
        uart_putc('\n');
        uart_b2x(recv[0]);
        uart_putc(' ');
        uart_b2x(recv[1]);
        uart_putc(' ');
        uart_b2x(recv[2]);
        uart_putc(' ');
        uart_b2x(recv[3]);
        uart_putc(' ');
        uart_b2x(kernel_size);
        uart_putc('\n');
    }*/
    
    /*while(1){
        uart_b2x(kernel_size);
        uart_putc('\n');
        uart_putc(uart_getc());
        uart_putc('\n');
    }*/

    while(kernel_size > 0){
        *kernel_addr = uart_getc_img();
        kernel_addr++;
        //uart_b2x(kernel_size);
        //uart_putc('\n');
        kernel_size--;
    }
    /*for(;kernel_size>0; kernel_size--){
        *kernel_addr = uart_getc_img();
        kernel_addr++;
        uart_b2x(kernel_size);000e14ff
        uart_putc('\n');
    }*/

    uart_puts("Kernel transmission completed\n");
    // x30 is link register(lr), record return address(usuallt used when bl is called)
    // here we use it as our path to our shell kernel as 'ret' will return to return address
    asm volatile(
        "mov x30, 0x80000;"
        "ret;"
    );
}