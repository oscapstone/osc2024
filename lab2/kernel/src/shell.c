#include "shell.h"
#include "uart.h"
#include "reboot.h"
#include "cpio.h"

int strcmp(const char* str1, const char* str2) {
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    if (*str1 == '\0' && *str2 =='\0') {
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
        if(element == '\n') {
            *input_ptr = '\0';
            uart_puts("\n");
            break;
        }
        *input_ptr++ = element;
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
        uart_puts("ls	: list all files\n");
        uart_puts("cat	: print file content\n");
        uart_puts("reboot	: print file content\n");
    } else if (strcmp(command_string,"ls")) {
        char *cpioDest = (char *)0x20000000;
        cpio_list((char*)cpioDest);
    } else if (strcmp(command_string,"cat")) {
        char filename[256];
        char *cpioDest = (char *)0x20000000;
        uart_puts("Enter filename: ");
        get_command(filename);
        cpio_cat((char*)cpioDest, filename);
    } else if (strcmp(command_string,"reboot")) {
        uart_puts("Rebooting....\n");
        reset(1000);
    }
}