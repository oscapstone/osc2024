#include "shell.h"
#include "string.h"
#include "uart.h"
#include "power.h"
#include "initrd.h"

void shell_init()
{
    uart_init();
    uart_puts("\n\n Hello from Raspi 3b+\n");
}

void shell_input(char *cmd)
{
    char c;
    int idx = 0, end = 0;

    while ((c = uart_getc()) != '\n') {
        uart_send(c);
        cmd[idx++] = c;
        cmd[++end] = '\0';
    }
}

void shell_controller(char *cmd)
{
    uart_send('\n');
    if (!strcmp(cmd, ""))
        return;
    else if (!strcmp(cmd, "help")) {
        uart_puts("help      : print this help menu\n");
        uart_puts("hello     : print Hello World!\n");
        uart_puts("ls        : list file in initramfs.cpio\n");
        uart_puts("cat       : show the file content\n");
        uart_puts("reboot    : reboot the device\n");
    } else if (!strcmp(cmd, "hello")) {
        uart_puts("Hello World!\n");
    } else if (!strcmp(cmd, "reboot")) {
        reset();
    } else if (!strcmp(cmd, "shutdown")) {
        power_off();
    } else if (!strcmp(cmd, "ls")) {
        initrd_ls();
    } else if (!strcmp(cmd, "ls -l")) {
        initrd_list();
    } else if (!strcmp(cmd, "cat")) {
        initrd_cat();
    } else {
        uart_puts("shell: command not found\n");
    }
}