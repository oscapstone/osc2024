#include "shell.h"
#include "cpio.h"
#include "dtb.h"
#include "uart.h"
#include "utils.h"

char command_buffer[CMD_MAX_LEN];
char *cmd_ptr;
int argc;
char argv[ARG_MAX_NUM][ARG_MAX_LEN];

extern void *CPIO_DEFAULT_PLACE;

void shell_main()
{
    uart_puts("Simple Shell\n");
    while (1)
    {
        // Reset CMD
        cmd_ptr = command_buffer;
        *cmd_ptr = '\0';

        // Reset ARG
        for (int i = 0; i < argc; i++)
            argv[i][0] = '\0';
        argc = 0;

        uart_puts("# ");
        read_cmd();

        if (!strcmp(command_buffer, ""))
            continue;
        else if (!strcmp(command_buffer, "help"))
            cmd_help();
        else if (!strcmp(command_buffer, "hello"))
            cmd_hello();
        else if (!strcmp(command_buffer, "reboot"))
            cmd_reboot();
        else if (!strcmp(command_buffer, "cat"))
            cmd_cat();
        else if (!strcmp(command_buffer, "ls"))
            cmd_ls();
        else if (!strcmp(command_buffer, "dtb"))
            cmd_dtb();
        else if (!strcmp(command_buffer, "clear"))
            uart_puts("\033[2J\033[H");
        else
        {
            uart_puts("Unknown command: ");
            uart_puts(command_buffer);
            uart_puts("\n");
        }
    }
}

void read_cmd()
{
    char c;
    while (1)
    {
        if (cmd_ptr - command_buffer >= CMD_MAX_LEN)
            break;

        c = uart_getc();

        if (c == '\n' || c == ' ')
            break;

        if (c < 32 || c > 126) // skip unwanted character
            continue;

        uart_send(c);
        *cmd_ptr++ = c;
    }

    uart_send(c);
    *cmd_ptr = '\0';

    if (c == ' ')
        read_arg();
}

void read_arg()
{
    // char c;
    while (1)
    {
        char *curr_arg = argv[argc];
        while (1)
        {
            *curr_arg = uart_getc();

            if (*curr_arg == '\n' || *curr_arg == ' ')
                break;

            uart_send(*curr_arg++);
        }

        if (*curr_arg == '\n')
        {
            uart_send(*curr_arg);
            *curr_arg = '\0';
            argc++;
            break;
        }

        *curr_arg = '\0';
        argc++;
    }
}

void cmd_help()
{
    uart_puts("help\t: print help menu\n");
    uart_puts("hello\t: print Hello World!\n");
    uart_puts("reboot\t: reboot device\n");
    uart_puts("ls\t: list files\n");
    uart_puts("cat\t: dump file\n");
    uart_puts("dtb\t: list device\n");
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

void cmd_cat()
{
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;
    cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // if parse header error
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if (strcmp(c_filepath, argv[0]) == 0)
        {
            char tmp = c_filedata[c_filesize];
            c_filedata[c_filesize] = '\0';
            uart_puts(c_filedata);
            uart_puts("\n");
            c_filedata[c_filesize] = tmp;
            break;
        }

        // if this is TRAILER!!! (last of file)
        if (header_ptr == 0)
        {
            uart_puts("cat:");
            uart_puts(argv[0]);
            uart_puts("  No such file or directory\n");
        }
    }
}

void cmd_ls()
{
    char *c_filepath;
    char *c_filedata;
    unsigned int c_filesize;
    cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while (header_ptr != 0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        // if parse header error
        if (error)
        {
            uart_puts("cpio parse error");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_ptr != 0)
        {
            uart_puts(c_filepath);
            uart_puts("\n");
        }
    }
}

void cmd_dtb()
{
    traverse_device_tree(dtb_callback_show_tree);
}
