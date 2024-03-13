#include "mini_uart.h"
// #include "mbox.h"


void help()
{
    // uart_send_string("The following commands are supported:\n");
    uart_send_string("help       : print this help menu\r\n");
    uart_send_string("hello      : print Hello World!\r\n");
    uart_send_string("reboot     : reboot the device\r\n");
    uart_send_string("mailbox    : print the hardware's information\r\n");
    // uart_send_string("loadimg - load an image to memory\n");
    // uart_send_string("exec - execute a loaded image\n");
    // uart_send_string("meminfo - display memory information\n");
    // uart_send_string("exit - exit the shell\n");
}

void hello()
{
    uart_send_string("Hello World!\r\n");
}

void reboot()
{
    uart_send_string("Rebooting...\r\n");
    reset();
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 != '\0' || *str2 != '\0') {
        if (*str1 < *str2) {
            return -1;
        } else if (*str1 > *str2) {
            return 1;
        }
        str1++;
        str2++;
    }
    return 0;
}

void parse_command(char *command)
{
    if (strcmp(command, "") == 0)
    {
        return;
    }
    else if (strcmp(command, "help") == 0)
    {
        help();
    }
    else if (strcmp(command, "hello") == 0)
    {
        hello();
    }
    else if (strcmp(command, "reboot") == 0)
    {
        reboot();
    }
    else if (strcmp(command, "mailbox") == 0)
    {
        get_board_revision();
        get_arm_memory();
        // if (mbox_call(MBOX_CH_PROP)) {
        //     uart_send_string("My serial number is: ");
        //     uart_hex(mbox[6]);
        //     uart_hex(mbox[5]);
        //     uart_send_string("\n");
        // } else {
        //     uart_send_string("Unable to query serial!\n");
        // }

    }
    else
    {
        for(int i = 0; command[i] != '\0'; i++) {
            uart_send(command[i]);
        }
        uart_send_string(": Command not found.\r\n");
    }
}
