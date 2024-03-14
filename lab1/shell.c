#include "shell.h"
#include "uart.h"
#include "mbox.h"
#include "reboot.h"

int str_cmp(const char *a, const char *b) {
    for(; *a!='\n'&&*b!='\0'; a++, b++)
        if(*a!=*b) return 0;
    if(*a=='\n'&&*b=='\0') return 1;
    return 0;
}

void shell() {
    char char_space[512];
    char *input=char_space;
    while(1) {
        uart_puts("# ");
        char c;
        while(1) {
            c=uart_getc();
            *input++=c;
            uart_send(c);
            if(c=='\n') {
                *input='\0';
                uart_puts("\r");
                break;
            }
        }

        input=char_space;
        if(str_cmp(input, "help")) {
            uart_puts("help    :print this help menu\n");
            uart_puts("hello   :print Hello World!\n");
            uart_puts("info    :print hardware info\n");
            uart_puts("reboot  :reboot device\n");
        }
        else if(str_cmp(input, "hello")) {
            uart_puts("Hello World!\n");
        }
        else if(str_cmp(input, "info")) {
            if(get_board_revision()) {
                uart_puts("Board revision: ");
                uart_hex(mbox[5]);
                uart_puts("\r\n");
            }
            else {
                uart_puts("Unable to query board revision.\n");
            }
            if(get_arm_memory()) {
                uart_puts("ARM memory base address: ");
                uart_hex(mbox[5]);
                uart_puts("\r\n");
                uart_puts("ARM memory size: ");
                uart_hex(mbox[6]);
                uart_puts("\r\n");
            }
            else {
                uart_puts("Unable to query ARM memory.\n");
            }
        }
        else if(str_cmp(input, "reboot")) {
            uart_puts("Rebooting...\n");
            reset(1000);
        }
        else {
            uart_puts("Command not found, try <help>\n");
        }
    }
}
