#include"header/uart.h"
#include"header/bootloader.h"
#include"header/shell.h"
char *dtb_base;
int relocated = 1;
void main(char *arg)
{
    uart_init();
    
    // register x0 
    dtb_base = arg;
    
    // relocate copies bootloader program from 0x80000 to 0x60000
    if (relocated) {
        relocated = 0;
        relocate(arg);
    }
    uart_send_str("\x1b[2J\x1b[H");
    shell(dtb_base);
}