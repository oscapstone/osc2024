#include "shell.h"
#include "uart1.h"
#include "power.h"
#include "u_string.h"

#define SHIFT_ADDR 0x100000

extern char* _dtb;
extern char _start[];

struct CLI_CMDS cmd_list[CLI_MAX_CMD]=
{
    {.command="loadimg", .help="load image via uart1"},
    {.command="help", .help="print all available commands"},
    {.command="reboot", .help="reboot the device"}
};

void cli_cmd_clear(char* buffer, int length)
{
    for(int i=0; i<length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char* buffer)
{
    char c='\0';
    int idx = 0;
    while(1)
    {
       if ( idx >= CMD_MAX_LEN ) break;

        c = uart_recv();

        // if user key 'enter'
        if ( c == '\n')
        {
            uart_puts("\r\n");
            break;
        }

        // if user key 'backspace'
        if ( c == '\b' || c == 127 )
        {
            if ( idx > 0 )
            {
                uart_puts("\b \b");
                idx--;
                buffer[idx] = '\0';
            }
            continue;
        }

        // use tab to auto complete
        if ( c == '\t' )
        {
            for(int tab_index = 0; tab_index < CLI_MAX_CMD; tab_index++)
            {
                if (strncmp(buffer, cmd_list[tab_index].command, strlen(buffer)) == 0)
                {
                    for (int j = 0; j < strlen(buffer); j++)
                    {
                        uart_puts("\b \b");
                    }
                    uart_puts(cmd_list[tab_index].command);
                    cli_cmd_clear(buffer, strlen(buffer) + 3);
                    strcpy(buffer, cmd_list[tab_index].command);
                    break;
                }
            }
            continue;
        }

        // some ascii blacklist
        if ( c > 16 && c < 32 ) continue;
        if ( c > 127 ) continue;

        buffer[idx++] = c;
        uart_send(c);
    }
}

void cli_cmd_exec(char* buffer)
{
    if (strcmp(buffer, "loadimg") == 0) {
        do_cmd_loadimg();
    } else if (strcmp(buffer, "help") == 0) {
        do_cmd_help();
    } else if (strcmp(buffer, "reboot") == 0) {
        do_cmd_reboot();
    } else if (*buffer){
        uart_puts(buffer);
        uart_puts(": command not found\r\n");
    }
}

void cli_print_banner()
{
    uart_puts("\r\n");
    uart_puts("=======================================\r\n");
    uart_puts("    OSC 2024 Shell Lab2 - Bootloader   \r\n");
    uart_puts("=======================================\r\n");
}

void do_cmd_help()
{
    for(int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
}

/* Overwrite image file into _start,
   Please make sure this current code has been relocated. */
void do_cmd_loadimg()
{
    char* bak_dtb = _dtb;
    char c;
    unsigned long long kernel_size = 0;
    char* kernel_start = (char*) (&_start);
    uart_puts("Please upload the image file.\r\n");

    // to get the kernel size for each row data
    for (int i=0; i<8; i++)
    {
        c = uart_getc();
        kernel_size += c<<(i*8);
    }

    // get the kernel data
    for (int i=0; i<kernel_size; i++)
    {
        c = uart_getc();
        kernel_start[i] = c;
    }
    uart_puts("Image file downloaded successfully.\r\n");
    uart_puts("Point to new kernel ...\r\n");

    ((void (*)(char*))kernel_start)(bak_dtb);
}

void do_cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int* rst_addr = (unsigned int*)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int* wdg_addr = (unsigned int*)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}

