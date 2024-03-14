#include "string.h"
#include "shell.h"
#include "uart.h"
#include "string.h"



void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void shell_start ()
{
    int buffer_counter = 0;
    char input_char;
    char buffer[MAX_BUFFER_LEN];

    strset (buffer, 0, MAX_BUFFER_LEN);

    // new line head
    uart_puts("# ");

    // read input
    while(1)
    {
        input_char = uart_getc();
        command_controller (input_char, buffer, &buffer_counter);
    }
}

void command_controller (char c, char buffer[], int * counter )
{
    if ( !(c < 128 && c >= 0) )
    {
    	return;
    }
    else if (c == LINE_FEED || c == CRRIAGE_RETURN)
    {
        uart_send(c);

        if ( (*counter) == MAX_BUFFER_LEN )
        {
            uart_puts("Reach Max Buffer Length!\n");
        }
        else
        {
            buffer[(*counter)] = '\0';

            if (!strcmp(buffer, "help"))
            {
            	uart_puts("help        : print this help menu\n");
    		uart_puts("hello       : print Hello World!\n");
   		uart_puts("reboot      : reboot the device\n");
            } 
            else if (!strcmp(buffer,"hello"))
            {
            	uart_puts("Hello World!\n");
            } 
            else if (!strcmp(buffer,"reboot"))
            {
            	uart_puts("Start rebooting...\n");
    		set(PM_RSTC, PM_PASSWORD | 0x20);
            }
            else
            {
            	uart_puts("Error: Command not found\n");
    		uart_puts("Try command: help\n");
            }                                        
        }

        (*counter) = 0;
        strset (buffer, 0, MAX_BUFFER_LEN);

        // new line head;
        uart_puts("# ");
    }
    else
    {
        uart_send(c);

        if ( *counter < MAX_BUFFER_LEN)
        {
            buffer[*counter] = c;
            (*counter) ++;
        }
    }
}
