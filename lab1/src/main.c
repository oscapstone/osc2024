#include "mini_uart.h"
#include "shell.h"

void main(void)
{   
    char username[50];
    char input_buffer[CMD_MAX_LEN];
    uart_init();
    uart_puts("\nInput Your Username >> ");
    shell_cmd_read(*username);
    shell_banner();
    
    while (1) {
        shell(*input_buffer);
    }
}
