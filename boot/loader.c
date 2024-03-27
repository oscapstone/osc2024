#include "loader.h"
#include "string.h"
#include "uart.h"

#define MAX_STR_LEN 1000

// load kernel by UART
void load_kernel()
{
    // 1. receive start header ('MC' + size + 'CM' + \n)
    char start_hdr[MAX_STR_LEN];
    char c;
    unsigned int index = 0;
    uart_puts("Load Kernel...\n");
    do
    {
        c = (char) uart_getc();
        start_hdr[index++] = c;
    } while (c != '\n');
    start_hdr[index+1] = '\0';

    uart_puts("Received start_hdr...\n");

    // 2. check if header valid and kernel size
    int kernel_size = 0;
    kernel_size = get_kernel_size(start_hdr);
    if (!kernel_size) {
        return;
    }

    // // 
    // uart_puts("valid!\n");
    // 2. receive kernel, write to 0x80000
    char *kernel = (char *) 0x80000;
    for (int i = 0; i < kernel_size; i++) {
        kernel[i] = uart_getc();
    }

    uart_puts("Kernel is loaded!\n");

    // 4. jmp to 0x80000
    asm volatile(
		"mov x0,x27;"
		"mov x30,#0x80000;"
		"ret"	
	);
}

int get_kernel_size(const char *start_hdr)
{
    int hdr_len = strlen(start_hdr);

    if (!(start_hdr[0] == 'M' && start_hdr[1] == 'C')) {
        return 0;
    }
    if (!(start_hdr[hdr_len-1] == 'M' && start_hdr[hdr_len-2] == 'C')) {
        return 0;
    }

    int kernel_size = 0;
    for (int i = 2; i < hdr_len-2; i++) {
        kernel_size = 10 * kernel_size + (start_hdr[i]-'0');
    }

    return kernel_size;
}