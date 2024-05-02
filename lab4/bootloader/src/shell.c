#include "shell.h"
#include "mbox.h"
#include "power.h"
#include "stdio.h"

extern unsigned long long _start;
char *dtb;

struct CLI_CMDS cmd_list[CLI_MAX_CMD] = {
    {.command = "hello", .help = "print Hello World!", .func = do_cmd_hello},
    {.command = "help", .help = "print all available commands", .func = do_cmd_help},
    {.command = "info", .help = "get device information via mailbox", .func = do_cmd_info},
    {.command = "loadimg", .help = "load image via uart1", .func = do_cmd_loadimg},
    {.command = "reboot", .help = "reboot the device", .func = do_cmd_reboot},
    {.command = "reboot_cancel", .help = "cancel reboot", .func = do_cmd_cancel_reboot}};

int start_shell(char *_dtb)
{
    dtb = _dtb;
    char input_buffer[CMD_MAX_LEN];
    shell_init();
    cli_print_banner();
    while (1)
    {
        cli_flush_buffer(input_buffer, CMD_MAX_LEN);
        puts("[ ٩(´ᗜ`*)୨みーちゃん ] $ ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
    return 0;
}

void shell_init()
{
    cmd_list[0].func = do_cmd_hello;
    cmd_list[1].func = do_cmd_help;
    cmd_list[2].func = do_cmd_info;
    cmd_list[3].func = do_cmd_loadimg;
    cmd_list[4].func = do_cmd_reboot;
    cmd_list[5].func = do_cmd_cancel_reboot;
}

void cli_flush_buffer(char *buffer, int length)
{
    for (int i = 0; i < length; i++)
    {
        buffer[i] = '\0';
    }
};

void cli_cmd_read(char *buffer)
{
    char c = '\0';
    int idx = 0;
    while (idx < CMD_MAX_LEN - 1)
    {
        c = getchar();
        if (c == 127) // backspace
        {
            if (idx != 0)
            {
                puts("\b \b");
                idx--;
            }
        }
        else if (c == '\n')
        {
            break;
        }
        else if (c <= 16 || c >= 32 || c < 127)
        {
            putchar(c);
            buffer[idx++] = c;
        }
    }
    buffer[idx] = '\0';
    puts("\r\n");
}

void cli_cmd_exec(char *buffer)
{
    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        if (strcmp(buffer, cmd_list[i].command) == 0)
        {
            cmd_list[i].func();
            return;
        }
    }
    if (*buffer)
    {
        puts(buffer);
        puts(": command not found\r\n");
    }
}

void cli_print_banner()
{
    puts("            ,.  ,.                                                 \r\n");
    puts("            ||  ||        _____     _ _ _____         _            \r\n");
    puts("           ,''--''.      |   __|___| | |     |___ ___| |___        \r\n");
    puts("          : (.)(.) :     |   __| .'| | | | | | .'| . | | -_|       \r\n");
    puts("         ,'        `.    |__|  |__,|_|_|_|_|_|__,|  _|_|___|       \r\n");
    puts("         :          :                            |_|               \r\n");
    puts("         :          :                                              \r\n");
    puts("   -ctr- `._m____m_,'         https://github.com/HiFallMaple       \r\n");
    puts("                                                                   \r\n");
}

int do_cmd_help()
{
    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        puts(cmd_list[i].command);
        puts("\t\t\t: ");
        puts(cmd_list[i].help);
        puts("\r\n");
    }
    return 0;
}

int do_cmd_hello()
{
    puts("Hello World!\r\n");
    return 0;
}

int do_cmd_info()
{
    // print hw revision
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        puts("Hardware Revision\t: 0x");
        // put_hex(pt[6]);
        put_hex(pt[5]);
        puts("\r\n");
    }
    // print arm memory
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_ARM_MEMORY;
    pt[3] = 8;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        puts("ARM Memory Base Address\t: 0x");
        put_hex(pt[5]);
        puts("\r\n");
        puts("ARM Memory Size\t\t: 0x");
        put_hex(pt[6]);
        puts("\r\n");
    }
    return 0;
}

int do_cmd_reboot()
{
    puts("Reboot in 10 seconds ...\r\n\r\n");
    volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 0x160000;
    return 0;
}

int do_cmd_cancel_reboot()
{
    volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x0;
    volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 0x0;
    return 0;
}

int do_cmd_loadimg()
{
    char c;
    unsigned long long kernel_size = 0;
    char *kernel_start = (char *)(&_start);
    puts("Please upload the image file.\r\n");
    for (int i = 0; i < 8; i++)
    {
        c = getbin();
        kernel_size += c << (i * 8);
    }
    for (int i = 0; i < kernel_size; i++)
    {
        c = getbin();
        kernel_start[i] = c;
    }
    puts("Image file downloaded successfully.\r\n");
    puts("Point to new kernel ...\r\n");

    ((void (*)(char *))kernel_start)(dtb);
    return 0;
}
