#include "uart.h"
#include "shell.h"
#include "utils.h" //add later
#include "dtb.h" //add later

#define MEMORY_POOL_SIZE 1024

static char memory_pool[MEMORY_POOL_SIZE];
static char *next_free = memory_pool;

void *simple_alloc(int size) {
    if (next_free + size - memory_pool > MEMORY_POOL_SIZE) {
        return 0; 
    }
    void *allocated = next_free; 
    next_free += size; 
    return allocated; 
}


void main(void *dtb)
{
    // set up serial console
    uart_init();
    // press any key to start
    uart_puts("Press any key to start!\n");
    uart_getc();
    //dtb_list(dtb);
    // say hello
    uart_puts("Booted, start testing simple_alloc\n");

    uart_puts("Command: char* string = simple_alloc(8);\n");
    char* string = simple_alloc(8);
    uart_hex((unsigned int) string);
    uart_puts("\n");
    uart_send('\r');
    string[0] = '1';
    string[1] = '2';
    string[2] = '3';
    string[3] = '4';
    string[4] = '5';
    string[5] = '6';
    string[6] = '7';
    string[7] = '\0';
    uart_puts(string);
    uart_puts("\n");
    uart_send('\r');

    char* string2 = simple_alloc(8);
    uart_hex((unsigned int) string2);
    uart_puts("\n");
    uart_send('\r');

    //dtb
    uart_puts("initrd before callback:");
	uart_hex(get_initramfs());
	uart_puts("\nfind dtb from ");
	uart_hex(dtb);
	find_dtb(dtb, "linux,initrd-start", 18, &callback_initramfs);

	uart_puts("\ninitrd after callback:");
	uart_hex(get_initramfs());
	uart_puts("\n");

    int idx = 0;
    char in_char;
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