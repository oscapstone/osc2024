#include "shell.h"
#include "uart1.h"
#include "power.h"
#include "utils.h"

#define SHIFT_ADDR 0x100000

extern char *_dtb;
extern char _start[];

struct CLI_CMDS cmd_list[CLI_MAX_CMD] =
    {
        {.command = "loadimg", .help = "load image via uart1"},
        {.command = "help", .help = "print all available commands"},
        {.command = "reboot", .help = "reboot the device"}};

void cli_cmd_clear(char *buffer, int length)
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
    while (1)
    {
        if (idx >= CMD_MAX_LEN)
            break;

        c = uart_recv();
        if (c == '\n')
        {
            uart_puts("\r\n");
            break;
        }
        if (c == 127 && idx > 0)
        {
            uart_puts("\b \b");
            idx--;
        }
        else
        {
            buffer[idx++] = c;
            uart_send(c);
        }
    }
}

void cli_cmd_exec(char *buffer)
{
    if (strcmp(buffer, "loadimg") == 0)
    {
        cmd_loadimg();
    }
    else if (strcmp(buffer, "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(buffer, "reboot") == 0)
    {
        cmd_reboot();
    }
    else if (*buffer)
    {
        uart_puts(buffer);
        uart_puts(": command not found\r\n");
    }
}

void cli_print_banner()
{
    uart_puts("\r\n");
    uart_puts("Bootloader\r\n");
}

void cmd_help()
{
    for (int i = 0; i < CLI_MAX_CMD; i++)
    {
        uart_puts(cmd_list[i].command);
        uart_puts("\t\t: ");
        uart_puts(cmd_list[i].help);
        uart_puts("\r\n");
    }
}

/* Overwrite image file into _start,
   Please make sure this current code has been relocated. */
void cmd_loadimg()
{
    char *bak_dtb = _dtb;                   // Backup the original DTB pointer
    char c;                                 // Variable to store received characters
    unsigned long long kernel_size = 0;     // Variable to store the size of the kernel image
    char *kernel_start = (char *)(&_start); // Pointer to the start of the kernel in memory

    uart_puts("Please upload the image file.\r\n");

    // Read the first 8 bytes to get the kernel size (assumes little-endian format)
    for (int i = 0; i < 8; i++)
    {
        c = uart_getc();             // Read a character from UART
        kernel_size += c << (i * 8); // Accumulate the size, shifting each byte to its correct position
    }

    // Read the kernel image into memory, byte by byte
    for (int i = 0; i < kernel_size; i++)
    {
        c = uart_getc();     // Read a character from UART
        kernel_start[i] = c; // Store the character in the kernel memory area
    }

    uart_puts("Image file downloaded successfully.\r\n"); // Inform the user that the download is complete
    uart_puts("Point to new kernel ...\r\n");             // Inform the user that the system is switching to the new kernel

    // Jump to the new kernel, passing the original DTB pointer as an argument
    ((void (*)(char *))kernel_start)(bak_dtb);
}

void cmd_reboot()
{
    uart_puts("Reboot in 5 seconds ...\r\n\r\n");
    volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
    *rst_addr = PM_PASSWORD | 0x20;
    volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
    *wdg_addr = PM_PASSWORD | 5;
}
