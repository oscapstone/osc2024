#include "cmd.h"
#include "alloc.h"
#include "initrd.h"
#include "mbox.h"
#include "mm.h"
#include "shell.h"
#include "string.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"

struct command commands[] = {
    { .name = "help", .help = "print this help menu", .func = help },
    { .name = "hello", .help = "print Hello World!", .func = hello },
    { .name = "reboot", .help = "reboot the device", .func = reboot },
    { .name = "lshw", .help = "list hardware information", .func = lshw },
    { .name = "ls", .help = "list ramdisk files", .func = ls },
    { .name = "cat", .help = "print ramdisk file", .func = cat },
    { .name = "exec", .help = "execute a program", .func = exec },
    { .name = "clear", .help = "clear the screen", .func = clear },
    { .name = "timeout", .help = "print mesg after duration", .func = timeout },
    { .name = "demo", .help = "demo other features", .func = demo },
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
        uart_puts("\t| ");
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
    // FIXME: Extract the functions to mbox.c
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
    char buffer[SHELL_BUF_SIZE];
    uart_puts("Filename: ");
    read_user_input(buffer);

    initrd_cat(buffer);
}

void exec()
{
    // Get filename from user input
    char buffer[SHELL_BUF_SIZE];
    uart_puts("Filename: ");
    read_user_input(buffer);

    initrd_exec(buffer);
}

void clear()
{
    uart_puts("\033[2J\033[H");
}

void timeout()
{
    // TODO: Replace with kmalloc
    char *msg = simple_malloc(SHELL_BUF_SIZE);
    uart_puts("Message: ");
    read_user_input(msg);

    char sec[SHELL_BUF_SIZE];
    uart_puts("Seconds: ");
    read_user_input(sec);

    strcat(msg, "\n"); // Add newline at the end
    set_timeout(msg, atoi(sec));
}

void demo()
{
    char select[SHELL_BUF_SIZE];
    uart_puts("(1) UART async write\n");
    uart_puts("(2) UART async read\n");
    uart_puts("(3) Buddy system\n");
    uart_puts("(4) Dynamic allocator\n");
    uart_puts("Select: ");
    read_user_input(select);
    switch (atoi(select)) {
    case 1:
        uart_async_write("[INFO] Test the UART async write function\n");
        break;
    case 2:
        uart_puts("Enter text:\n");
        uart_enable_rx_interrupt();
        // Note: Set a longer delay for testing in QEMU
        for (int i = 0; i < 10000000; i++)
            ;
        char buffer[256];
        uart_async_read(buffer, 256);
        uart_puts(buffer);
        uart_putc('\n');
        break;
    case 3:
        uart_puts("Get 2 pages 3 times\n");
        void *p31 = kmalloc(8192);
        void *p32 = kmalloc(8192);
        void *p33 = kmalloc(8192);
        kfree(p31);
        kfree(p32);
        kfree(p33);
        break;
    case 4:
        uart_puts("Allocate 100 bytes\n");
        void *p41 = kmalloc(100);
        uart_puts("p1 = ");
        uart_hex((uintptr_t)p41);
        uart_puts("\n");
        kfree(p41);
        break;
    default:
        uart_puts("Option not found.\n");
        break;
    }
}