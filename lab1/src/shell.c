#include "shell.h"
#include "uart.h"
#include "reboot.h"

int strcmp(const char* str1, const char* str2) {
    while (*str1 != '\n' || *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    if (*str1 == '\n' && *str2 =='\0') {
        return 1;
    } else {
        return 0;
    }
}

void get_command(char *command_string){
    char element;
    char *input_ptr = command_string; 
    while(1) {
        element = uart_getc();
        *input_ptr++ = element;
        if(element == '\n') {
            *input_ptr = '\0';
            uart_puts("\n");
            break;
        }
        uart_send(element);
    }
}

void shell(){
    char command_string[256];
    uart_puts("# ");
    // get command
    get_command(command_string);
    // read command
    if(strcmp(command_string,"help")) {
        uart_puts("help	: print this help menu\n");
        uart_puts("hello	: print Hello World!\n");
        uart_puts("reboot	: reboot the device\n");
    } else if (strcmp(command_string,"hello")) {
        uart_puts("Hello World!\n");
    } else if (strcmp(command_string,"reboot")) {
        uart_puts("Rebooting....\n");
        reset(1000);
    }     
}