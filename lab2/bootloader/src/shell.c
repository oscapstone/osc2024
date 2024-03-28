#include "mini_uart.h"
#include "shell.h"
#include "reboot.h"

extern unsigned long long __begin;
extern char *_dtb;

int shell_cmd_strcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;

    do
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);
    return c1 - c2;
}

void shell_banner(void)
{
    uart_puts("\n==================Now is 2024==================\n");
    uart_puts("||       Here comes the OSC 2024 Lab1         ||\n");
    uart_puts("===============================================\n");
}

void shell_cmd_read(char *buffer)
{
    char cursor = '\0';
    int index = 0;
    while (1)
    {
        if (index > CMD_MAX_LEN)
            break;

        cursor = uart_recv();
        if (cursor == '\n')
        {
            uart_puts("\r\n");
            break;
        }
        if (cursor > 16 && cursor < 32)
            continue;

        if (cursor > 127)
            continue;
        buffer[index++] = cursor;
        uart_send(cursor);
    }
}

void shell_cmd_exe(char *buffer)
{
    if (shell_cmd_strcmp(buffer, "hello") == 0)
    {
        uart_puts("ヽ(́◕◞౪◟◕‵)ﾉ>> ");
        shell_hello_world();
    }
    else if (shell_cmd_strcmp(buffer, "help") == 0)
    {
        shell_help();
    }
    else if (shell_cmd_strcmp(buffer, "info") == 0)
    {
        shell_info();
    }
    else if (shell_cmd_strcmp(buffer, "reboot") == 0)
    {
        uart_puts("You have roughly 10 seconds to cancel reboot.\nCancel reboot with\nreboot -c\n");
        shell_reboot();
    }
    else if (shell_cmd_strcmp(buffer, "reboot -c") == 0)
    {
        shell_cancel_reboot();
    }
    else if (shell_cmd_strcmp(buffer, "loadimg") == 0)
    {
        shell_loadimg();
    }
    else if (*buffer)
    {
        uart_puts("(｡ŏ_ŏ) Oops! ");
        uart_puts(buffer);
        uart_puts(": command not found\r\n");
    }
}
void shell_loadimg()
{
    char *bak_dtb = _dtb;
    char c;
    unsigned long long kernel_size = 0;
    char *kernel_start = (char *)(&__begin);
    uart_puts("Please upload the image file.\r\n");
    for (int i = 0; i < 8; i++)
    {
        c = uart_get();
        kernel_size += c << (i * 8);
    }
    for (int i = 0; i < kernel_size; i++)
    {
        c = uart_get();
        kernel_start[i] = c;
    }
    uart_puts("Image file downloaded successfully.\r\n");
    uart_puts("Point to new kernel ...\r\n");

    ((void (*)(char *))kernel_start)(bak_dtb);

}

void shell_clear(char *buffer, int len)
{
    for (int i = 0; i < len; i++)
    {
        buffer[i] = '\0';
    }
}

void shell(char *input_buffer)
{
    shell_clear(input_buffer, CMD_MAX_LEN);
    uart_puts("ヽ(✿ﾟ▽ﾟ)ノ >> \t");
    shell_cmd_read(input_buffer);
    shell_cmd_exe(input_buffer);
}

void shell_hello_world()
{
    uart_puts("hello world!\n");
}

void shell_help()
{
    uart_puts("help     : print this help menu.\n");
    uart_puts("hello    : print hello world!\n");
    uart_puts("loadimg  : Receive Image!\n");
    uart_puts("clear    : clear screen.\n");
    uart_puts("reboot   : reboot raspberry pi.\n");
}

void shell_info()
{
    unsigned int board_revision;
    get_board_revision(&board_revision);
    uart_puts("Board revision is : 0x");
    uart_hex(board_revision);
    uart_puts("\n");

    unsigned int arm_mem_base_addr;
    unsigned int arm_mem_size;

    get_arm_memory_info(&arm_mem_base_addr, &arm_mem_size);
    uart_puts("ARM memory base address in bytes : 0x");
    uart_hex(arm_mem_base_addr);
    uart_puts("\n");
    uart_puts("ARM memory size in bytes : 0x");
    uart_hex(arm_mem_size);
    uart_puts("\n");

    uart_puts("\n");
    uart_puts("This is a simple shell for raspi3.\n");
    uart_puts("type help for more information\n");
}

void shell_reboot()
{
    // uart_puts("reboot not yet \n");
    reset(196608);
}

void shell_cancel_reboot()
{
    cancel_reset();
    uart_puts("reboot canceled. \n");
}