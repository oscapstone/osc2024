
#include "load.h"

void cmd(const char *s1) {
    
    if(!strcmp(s1, "help")) {
        uart_puts("help   \t: print this help menu\n");
        uart_puts("hello  \t: print Hello World!\n");
        uart_puts("load   \t: load kernel image through uart\n");
        uart_puts("reboot \t: reboot the device\n");
    }
    else if(!strcmp(s1, "hello")) {
        uart_puts("Hello World!\n");
    }
    else if(!strcmp(s1, "load")) {
        load();
    }
    else {
        uart_puts("Unknown command: ");
        uart_puts(s1);
        uart_puts("\n");
    }
}

void shell() {
    while(1) {
        uart_puts("# ");
        //uart_send(uart_getc());
        int i = 0;
        char str[50] = {};
        char c = ' ';
        
        while( c != '\n') {
            c = uart_getc();

            if(c == '\n') {
                uart_puts("\n");
            }
            else {
                uart_send(c);
            }
            if(c == 0x08 || c == 0x7f && i > 0) {
                uart_send('\b');
                uart_send(' ');
                uart_send('\b');
                str[strlen(str) - 1] = '\0';
                --i;
            }
            
            if(c != '\n' && c != 0x08 && c != 0x7f)
                str[i++] = c;

        }
        cmd(str);

    }
}