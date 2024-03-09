#include "shell.h"
#include "uart.h"
#include "utils.h"

int i = 0;
char cmd[CMD_MAX_LEN];

void shell_main()
{
    uart_puts("Simply Shell\n");
    while (1)
    {
        uart_puts("#");
        read_cmd();
        
        if (strcmp(cmd, "help"))
            cmd_help();
        else if (strcmp(cmd, "hello"))
            cmd_hello();
        else if (strcmp(cmd, "reboot"))
            cmd_reboot();
        // else if (strcmp(cmd, "exit"))
        //     break;
        else
        {
            uart_puts("Unknown command: ");
            uart_puts(cmd);
            uart_puts("\n");
        }
    }
}

void read_cmd()
{
    while (1)
    {
        if (i >= CMD_MAX_LEN)
            break;

        cmd[i] = uart_getc();

        if (cmd[i] == '\n')
        {
            uart_puts("\n");
            break;
        }

        uart_send(cmd[i++]);
    }
    cmd[i] = '\0';
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
    *w = PM_PASSWORD | 5;
}

// void cancel_reset()
// {
//     set(PM_RSTC, PM_PASSWORD | 0); // full reset
//     set(PM_WDOG, PM_PASSWORD | 0); // number of watchdog tick
// }