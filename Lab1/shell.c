#include "mbox.h"
#include "reboot.h"
#include "uart.h"

int strcmp(char *s1, char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}


void shell(char * cmd){
    if(strcmp(cmd, "help") == 0){
        uart_puts("\rhelp\t: print all available commands\n");
        uart_puts("hello\t: print Hello World!\n");
        uart_puts("mailbox\t: print hardware info\n");
        uart_puts("reboot\t: reboot raspberry 3b+\n\r");
    }
    else if(strcmp(cmd, "hello") == 0){
        uart_puts("\rHello World!\n\r");
    }
    else if(strcmp(cmd, "mailbox") == 0){
        if(get_board_revision()){
            uart_puts("\rBoard Revision:\t\t\t0x");
            uart_hex(mailbox[5]);
            uart_puts("\n\r");
        }
        else{
            uart_puts("Get Revision Failed!\n\r");
        }
        
        if(get_arm_memory()){
            uart_puts("ARM Memory Base Address:\t0x");
            uart_hex(mailbox[5]);
            uart_puts("\n\r");
            uart_puts("ARM Memory size:\t\t0x");
            uart_hex(mailbox[6]);
            uart_puts("\n\r");
        }
        else{
            uart_puts("Get Memory Failed!\n\r");
        }
    }
    else if(strcmp(cmd, "reboot") == 0){
        uart_puts("\r");
        reset(5);
    }
    else{
        uart_puts("\r");
    }
}