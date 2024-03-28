#include "header/uart.h"
#include "header/bootloader.h"

void read_cmd(char* command) 
{
    char input_char;
    int idx = 0;
    command[0] = '\0';
    while((input_char = uart_get_char()) != '\n') {
        command[idx] = input_char;
        idx += 1;
        uart_send_char(input_char);
    }
    command[idx] = '\0';
}

int strcmp(const char* command, const char* b)
{
    while(*command != '\0' && *b != '\0') {
        if(*command != *b) {
            return 0;
        }
        command++;
        b++;
    }
    return (*command == '\0' && *b == '\0');
}

void shell()
{
    uart_send_string("\nBootLoaderShell> ");
    char command[256];
    read_cmd(command);
    if(strcmp(command, "help")) 
    {
        uart_send_string("\nhelp    : print this help menu ");
        uart_send_string("\nhello   : print Hello World! ");
        uart_send_string("\nbooting : load img ");
    }
    else if(strcmp(command, "hello")) 
    {
        uart_send_string("\nHello World! ");
    }
    else if (strcmp(command, "booting"))
    {
        uart_send_string("\nstart booting . . .");
        load_img();
    }
    else 
    {
        uart_send_string("\nUnknown command ");
    }
    
}

void main() {
    // set up serial console
    uart_init();
    // a shell for load

    while(1) {
        shell();
    }
}