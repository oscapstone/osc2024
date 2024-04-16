#include "bootloader.h"

uint32_t get_kernel_size(){
    
    uint32_t size = uart_read() << 24;
    size |= uart_read() << 16;
    size |= uart_read() << 8;
    size |= uart_read();

    print_str("\nKernel Size: ");
    print_hex(size);

    return size;
}

void load_kernel(){

    uint8_t* kernel = (uint8_t*)KERNEL_BASE;
    uint32_t kernel_size = get_kernel_size();

    print_str("\nLoading Kernel...\n");

    for (int i = 0; i < kernel_size; i++){
        kernel[i] = uart_read_bin();
        if ((i+1)%1024 == 0){
            print_hex(i);
            print_str("\n");
        }
    }

    print_str("\nKernel has been loaded\n");
    print_str("\nStart Booting...\n");

    asm volatile(
        "mov x30, 0x80000;"
        "ret;"
    );
}
