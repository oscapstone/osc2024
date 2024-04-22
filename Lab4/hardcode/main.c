#include "uart.h"
#include "shell.h"
#include "dtb.h"
#include "memory.h"

void main(void *dtb)
{
    // set up serial console
    uart_init();
    fdt_tranverse(dtb, "linux,initrd-start", initramfs_start_callback);
    fdt_tranverse(dtb, "linux,initrd-end", initramfs_end_callback);

    // say hello
    frames_init();

    uart_puts("Demo page alloc, press anytihng to continue!\n\r");
    uart_getc();
    demo_page_alloc();

    uart_puts("Init memory, press anything to continue!\n\r");
    uart_getc();
    init_memory();

    uart_puts("Start testing malloc and free.\n\r");
    uart_getc();
    int i = 500;
    // uart_hex(&i);
    // uart_puts("\n");
    while(i--){ // testing the linked list
        char * test2 = malloc(1);
    }
        
    char * test = malloc(14);
    uart_hex(test);
    test[0] = 't';
    test[1] = 'e';
    test[2] = 's';
    test[3] = 't';
    test[4] = '\0';
    uart_puts("\n\r");
    uart_puts(test);
    uart_puts("\n\r");
    free(test);
    char* test3 = malloc(14);
    test3 = malloc(14);
    test3 = malloc(14);
    test3 = malloc(80);

    char in_char;
    int idx = 0;

    // echo everything back
    while(1) {
        char buffer[1024];
        uart_send('\r');
        uart_puts("# ");
        while(1){
            in_char = uart_getc();
            uart_send(in_char);
            if(in_char == '\n'){
                buffer[idx] = '\0';
                shell(buffer);
                idx = 0;
                break;
            }
            else{
                buffer[idx] = in_char;
                idx++;
            }
        }
    }
}