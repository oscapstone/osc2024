#include "utils.h"
#include "mini_uart.h"
#include "peripheral/pm.h"
#include "string.h"
#define BUFFER_SIZE 256

void read_command(char *cmd) 
{
    int i = 0;
    char c;

    while (1) {
    c = uart_recv();
    uart_send(c);
    if (c == '\n') {
      cmd[i] = '\0';
      return;
    } 
    else
      cmd[i++] = c;
    }
}

void help(void) 
{
    uart_send_string("Shell for Raspberry Pi 3\r\n\0");
    uart_send_string("Available commands:\r\n\0");
    uart_send_string("  help - display this information\r\n\0");
    uart_send_string("  hello - display hello world\r\n\0");
    uart_send_string("  reboot - reboot the system\r\n\0");
}

void hello(void)
{
    uart_send_string("Hello, world!\r\n\0");
}

void reset(int tick)
{
    uart_send_string("rebooting...\r\n\0");
    put32( PM_RSTC, PM_PASSWORD | 0x20 ); // full reset
    put32 ( PM_WDOG, PM_PASSWORD | tick ); // number of watchdog tick
}

void cancel_reset(void)
{
    uart_send_string("reboot cancelled\r\n\0");
    put32( PM_RSTC, PM_PASSWORD | 0 ); // full reset
    put32 ( PM_WDOG, PM_PASSWORD | 0 ); // number of watchdog tick
}


void parse_command(char *cmd)
{
   if (!str_cmp(cmd, "help\0"))
      help();
   else if (!str_cmp(cmd, "hello\0"))
      hello();
   else if (!str_cmp(cmd, "reboot\0"))
      reset(10);
   else {
      uart_send_string("Command '\0");
      uart_send_string(cmd);
      uart_send_string("' not found\r\n\0");
    }
}

void shell(void)
{
    while (1) {
        char cmd[BUFFER_SIZE];
        uart_send_string("$ \0");
        read_command(cmd);
        parse_command(cmd);
    }
}
