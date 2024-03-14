#include "mini_uart.h"
#include "mailbox.h"
#include "reboot.h"
#include "io.h"
#include "string.h"

void readcmd(char *x)
{
    char input_char;
    int input_index = 0;
    x[0] = 0;
    while( ((input_char = read_char()) != '\n'))
    {
        x[input_index] = input_char;
        ++input_index;
        printfc(input_char);
    }

    x[input_index]=0; // null char
}

void shell()
{
    printf("\nyuchang@raspberrypi3: ~$ ");
    char command[256];
    readcmd(command);

    if( (strcmp(command, "hello") == 0) )
    {
        printf("\nHello, World!");
    }
    else if( (strcmp(command, "help") == 0) )
    {
        printf("\nhelp\t: print this help menu");
        printf("\nhello\t: print Hello, World!");
        printf("\nmailbox\t: print mailbox info");
        printf("\nreboot\t: reboot the device");
    }
    else if ( (strcmp(command, "mailbox") == 0) )
    {
        printf("\nMailbox info:");
        get_board_revision();
        get_memory_info();
    }
    else if( (strcmp(command, "reboot") == 0) )
    {
        printf("\nRebooting...\n");
        reset(200);
    }
    else
    {
        printf("\nCommand not found: ");
        printf(command);
    }
    
}

int main()
{
    uart_init();
    uart_send_string("\n Welcome to Yuchang's Raspberry Pi 3!\n");
    while(1)
    {
        shell();
    }
    return 0;
}