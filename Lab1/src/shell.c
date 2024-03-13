#include "shell.h"
#include "uart.h"
#include "utils.h"

char command_buffer[CMD_MAX_LEN];
char *cmd_ptr;

void shell_main()
{
    cmd_ptr = command_buffer;
    *cmd_ptr = '\0';

    uart_puts("Simple Shell\n");
    while (1)
    {
        uart_puts("# ");
        read_cmd();
        
        if (strcmp(command_buffer, "help"))
            cmd_help();
        else if (strcmp(command_buffer, "hello"))
            cmd_hello();
        else if (strcmp(command_buffer, "reboot"))
            cmd_reboot();
        // else if (strcmp(command_buffer, "exit"))
        //     break;
        else
        {
            uart_puts("Unknown command: ");
            uart_puts(command_buffer);
            uart_puts("\n");
        }

        // Reset CMD
        cmd_ptr = command_buffer;
        *cmd_ptr = '\0';
    }
}

void read_cmd()
{
    while (1)
    {
        if (cmd_ptr - command_buffer >= CMD_MAX_LEN)
            break;

        *cmd_ptr = uart_getc();

        if (*cmd_ptr == '\n')
        {
            uart_puts("\n");
            break;
        }

        if (*cmd_ptr < 20 || *cmd_ptr > 126) // skip unwanted character
            continue;

        uart_send(*cmd_ptr++);
    }
    *cmd_ptr = '\0';
}

void cmd_help()
{
    uart_puts("help\t: print this help menu\n");
    uart_puts("hello\t: print Hello World!\n");
    uart_puts("reboot\t: reboot the device\n");
}

void cmd_hello()
{
    uart_puts("Hello World!\n");
}

void cmd_reboot()
{
    uart_puts("Rebooting...\n\n");
    
    VUI *r = (UI *)PM_RSTC;
    *r = PM_PASSWORD | 0x20;
    VUI *w = (UI *)PM_WDOG;
    *w = PM_PASSWORD | 48;
}
