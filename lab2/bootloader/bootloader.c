#include "bootloader.h"
#include "mini_uart.h"
#include "utils.h"
#include "reboot.h"

#define MAX_BUFFER_SIZE 256u

static char buffer[MAX_BUFFER_SIZE];

void UARTbootloader() {
    int ksize = 0;
    char input;
    //char kernel_size[10];
    
    uart_send_string("Kernel code size is ");    
    while(1) {
        input = uart_recv();
        if(input == '\0') { break; }
        uart_send(input);
        ksize = (ksize * 10) + (input - '0');
    }
    uart_send_string(" bytes\r\n");

    uart_send_string("Receiving img\r\n");
    volatile char *kernel_pos =  (char*) 0x80000;
    for(int i = 0; i < ksize; i++) {
        input = uart_recv();
        kernel_pos[i] = input;
    }

    void (*switch_kernel)(void) = (void*) kernel_pos;
    for(int i = 0; i < 1000; i++) { asm volatile("nop"); }
    switch_kernel();
}

int BLstringcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *) p1;
    const unsigned char *s2 = (const unsigned char *) p2;
    unsigned char c1, c2;
    
    c1 = (unsigned char) *s1;
    c2 = (unsigned char) *s2;
    while(c1 == c2) {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if(c1 == '\0') { return c1 - c2; }
    }

    return c1 - c2;
}

void BLread_cmd()
{
    unsigned int idx = 0;
    char c = '\0';
    
    while (1) {
        c = uart_recv();
        if (c == '\r' || c == '\n') {
            uart_send_string("\r\n");
            
            if (idx < MAX_BUFFER_SIZE) buffer[idx] = '\0';
            else buffer[MAX_BUFFER_SIZE-1] = '\0';
            
            break;
        } 
        else {
            uart_send(c);
            buffer[idx++] = c;
        } 
    }

}

void BLparse_cmd()
{

    if (BLstringcmp(buffer, "\0") == 0) 
        uart_send_string("\r\n");
    else if (BLstringcmp(buffer, "reboot") == 0) {
        uart_send_string("rebooting...\r\n");
        reset(100);
    }
    else if (BLstringcmp(buffer, "loadimg") == 0) {
        UARTbootloader();
    }
    else if (BLstringcmp(buffer, "help") == 0) {
        uart_send_string("help:\t\tprint list of available commands\r\n");
        uart_send_string("reboot:\t\treboot the device\r\n");
        uart_send_string("loadimg:\t\tUART Bootloader\r\n");
    }
    else 
        uart_send_string("Command not found! Type help for commands.\r\n");

}

void Bootloader_main() {
    uart_init();
    uart_send_string("UART Bootloader Start!\r\n");
    while(1) {
        uart_send_string("# ");
        BLread_cmd();
        BLparse_cmd();
    }
}