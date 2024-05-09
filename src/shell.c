#include "shell.h"
#include "string.h"
#include "uart.h"
#include "power.h"
#include "initrd.h"
#include "demo.h"
#include "syscall.h"
#include "exception.h"
#include "exec.h"

#define CMD_LEN 128

/* Initialize UART and print Hello message. */
void shell_init()
{
    uart_init();
    uart_puts("\n\n Hello from Raspi 3b+\n==== init: UART\n");
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
        uart_puts("help           : print this help menu\n");
        uart_puts("hello          : print Hello World!\n");
        uart_puts("ls             : list file in initramfs.cpio\n");
        uart_puts("cat            : show the file content.\n");
        uart_puts("reboot         : reboot the device.\n");
        uart_puts("run            : load user program and run.\n");
        uart_puts("timer_on       : enable timer and print out current second periodiccally.\n");
        uart_puts("set_timeout    : set the timer to trigger an interrupt after given second.\n");
        uart_puts("demo_uart      : demo for asynchronous uart. Lab 3 Basic Exercise 3\n");
        uart_puts("demo_irq       : demo for bottom half irq. Lab 3 Advanced Exercise 2\n");
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
    } else if (!strcmp(cmd, "run")) {
        asm volatile ("svc 1");
    } else if (!strcmp(cmd, "timer_on")) {
        asm volatile ("svc 2");
    } else if (!strcmp(cmd, "set_timeout")) {
        asm volatile ("svc 3");
    } else if (!strcmp(cmd, "demo_uart")) {
        demo_async_uart();
    } else if (!strcmp(cmd, "demo_irq")) {
        asm volatile ("svc 5");
    } else {
        uart_puts("shell: command not found\n");
    }
}

/* do shell job, can be run in user mode or kernel mode. */
void do_shell_user(void)
{
    while (1) {
        uart_puts("# ");
        char cmd[CMD_LEN];
        shell_input(cmd);
        shell_controller(cmd);
    }
}

/* shell: kernel task. */
void do_shell(void)
{
    do_exec(do_shell_user);
}