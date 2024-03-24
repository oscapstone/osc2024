#include "mbox.h"
#include "mini_uart.h"
#include "reboot.h"
#include "string.h"

void shell_input(char *command)
{
    uart_send_string("# ");
    char c;
    int index = 0;
    command[0] = '\0';
    while ((c = uart_recv()) != '\n') //so that if we press enter, the sending process is done
    {
        command[index++] = c; //command[index] and then index+=1
        command[index] = '\0'; 
        uart_send(c);
    }
    uart_send_string("\r\n");
}

void shell_option(char *command)
{
    if (!strcmp(command, "")) //when command is null, do nothing
    {
        return;
    }

    else if (!strcmp(command, "help"))
    {
        uart_send_string("help\t\t: print this help menu\r\n");
        uart_send_string("hello\t\t: print Hello world!\r\n");
        uart_send_string("reboot\t\t: reboot the device\r\n");
        uart_send_string("board\t\t: show board revision\r\n");
    }

    else if (!strcmp(command, "hello"))
    {
        uart_send_string("Hello World!\r\n");
    }

    else if (!strcmp(command, "reboot")) //?
    {
        uart_send_string("raspi3 is rebooting...\r\n");
        reset(200);
        while (1)
        {
        }
    }

    else if (!strcmp(command, "board"))
    {
        get_board_revision();
        get_mem_info();
    }

    else
    {
        uart_send_string("unvalid command! try <help>\r\n");
    }
}
