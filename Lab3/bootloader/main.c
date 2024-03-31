#include "uart.h"
#include "io.h"
#include "bootloader.h"

#define BOOTLOADER_ADDRESS 0x60000
#define KERNEL_ADDRESS 0x80000

int main(){

    uart_init();

    print_str("\nBootLoader...");

    load_kernel();

    return 0;
}