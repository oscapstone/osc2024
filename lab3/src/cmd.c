#include "cmd.h"
#include "initrd.h"
#include "mbox.h"
#include "shell.h"
#include "string.h"
#include "uart.h"

struct command commands[] = {
    { .name = "help", .help = "print this help menu", .func = help },
    { .name = "hello", .help = "print Hello World!", .func = hello },
    { .name = "reboot", .help = "reboot the device", .func = reboot },
    { .name = "lshw", .help = "list hardware information", .func = lshw },
    { .name = "ls", .help = "list ramdisk files", .func = ls },
    { .name = "cat", .help = "print ramdisk file", .func = cat },
    { .name = "exec", .help = "execute a program", .func = exec },
    { .name = "NULL" } // Must put a NULL command at the end!
};

void help()
{
    int i = 0;

    while (1) {
        if (!strcmp(commands[i].name, "NULL")) {
            break;
        }
        uart_puts(commands[i].name);
        uart_puts("\t: ");
        uart_puts(commands[i].help);
        uart_putc('\n');
        i++;
    }
}

void hello()
{
    uart_puts("Hello World!\n");
}

void reboot()
{
#define PM_PASSWORD 0x5A000000
#define PM_RSTC     (volatile unsigned int *)0x3F10001C
#define PM_WDOG     (volatile unsigned int *)0x3F100024
    // Reboot after 180 ticks
    *PM_RSTC = PM_PASSWORD | 0x20; // Full reset
    *PM_WDOG = PM_PASSWORD | 180;  // Number of watchdog ticks
}

void lshw()
{
#define GET_BOARD_REVISION 0x00010002
    // Get board revision
    mbox[0] = 7 * 4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_BOARD_REVISION;
    mbox[3] = 4;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = END_TAG;
    if (mailbox_call(MAILBOX_CH_PROP)) {
        uart_puts("board revision: ");
        uart_hex(mbox[5]);
        uart_putc('\n');
    }

#define GET_ARM_MEM_INFO 0x00010005
    // Get ARM memory base address and size
    mbox[0] = 8 * 4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_ARM_MEM_INFO;
    mbox[3] = 8;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = 0;
    mbox[7] = END_TAG;
    if (mailbox_call(MAILBOX_CH_PROP)) {
        uart_puts("device base memory address: ");
        uart_hex(mbox[5]);
        uart_putc('\n');
        uart_puts("device memory size: ");
        uart_hex(mbox[6]);
        uart_putc('\n');
    }
}

void ls()
{
    initrd_list();
}

void cat()
{
    // Get filename from user input
    char buffer[MAX_BUF_SIZE + 1];
    uart_puts("Filename: ");
    read_user_input(buffer);

    initrd_cat(buffer);
}

void exec()
{
    // Get filename from user input
    char buffer[MAX_BUF_SIZE + 1];
    uart_puts("Filename: ");
    read_user_input(buffer);

    initrd_exec(buffer);
}