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
        else if (strcmp(command_buffer, "loadimg"))
            cmd_loadimg();
        else if (strcmp(command_buffer, "test"))
            cmd_test();
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
    uart_puts("loadimg\t: load new image\n");
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

extern char __start[];
extern char __end[];

void cmd_loadimg()
{
    // Backup old kernel
    char *t = (char *)TEMP_ADDR;
    char *s = __start;
    char *e = __end;

    for (; s <= e; s++, t++)
        *t = *s;

    uart_puts("finish copy\n");

    // Then jump to function done following task
    void (*func_ptr)() = cmd_loadimg2;
    unsigned long int func_addr = (unsigned long int)func_ptr;
    void (*function_call)(void) = (void (*)(void))(func_addr - (unsigned long int)__start + TEMP_ADDR);
    function_call();
}

void cmd_loadimg2()
{
    // load new kernel
    uart_puts("Kernel size: ");
    UI size = uart_getint();
    uart_puts("\n");

    char *s;
    s = (char *)KERNEL_ADDR;

    // char c;
    // char *cc = "a000\n\0";

    for (UI i = 0; i < size; i++)
    {
        // c = uart_getc();
        // *(s + i) = c;
        *s++ = uart_getc();
        // if (i % 1000 == 0)
        // {
        //     uart_puts(cc);
        //     cc[0]++;
        // }
    }

    uart_puts("finish load\n");

    // int cnt = 0;
    // char *ans = "asdfghjkl";
    // while (1)
    // {
    //     c[0] = uart_getc();
    //     uart_puts(c);

    //     if (c[0] == ans[cnt])
    //         cnt++;
    //     if (cnt == 9)
    //         break;
    // }

    uart_puts("\njump to new kernel\n");

    // jump to new kernel
    void (*new_kernel_start)(void) = (void *)KERNEL_ADDR;
    new_kernel_start();
}

void cmd_test()
{
    int size = uart_getint();
    uart_puts("Kernel size: ");

    int n = size;
    char c[2];
    c[1] = '\0';
    while (n)
    {
        c[0] = n % 10 + '0';
        uart_puts(c);
        n /= 10;
    }
    uart_puts("\n");
}
