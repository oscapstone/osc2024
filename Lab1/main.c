#include "uart.h"
#include "reboot.h"
#include "mbox.h"

int strcmp(char *s1, char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

void main()
{
    // set up serial console
    uart_init();
    
    // say hello
    uart_puts("Nice to meet you!\n\r");
    int idx = 0;
    char in_char;
    // echo everything back
    while(1) {
        char buffer[1024];
        uart_puts("\r# ");
        while(1){
            in_char = uart_getc();
            uart_send(in_char);
            if(in_char == '\n'){
                buffer[idx] = '\0';
                if(strcmp(buffer, "help") == 0){
                    uart_puts("\rhelp\t: print all available commands\n");
                    uart_puts("hello\t: print Hello World!\n");
                    uart_puts("mailbox\t: print hardware info\n");
                    uart_puts("reboot\t: reboot raspberry 3b+\n\r");
                }
                else if(strcmp(buffer, "hello") == 0){
                    uart_puts("\rHello World!\n\r");
                }
                else if(strcmp(buffer, "mailbox") == 0){
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
                else if(strcmp(buffer, "reboot") == 0){
                    uart_puts("\r");
                    reset(5);
                }
                else{
                    uart_puts("\r");
                }
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