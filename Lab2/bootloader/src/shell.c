#include "shell.h"
#include "load_img.h"
#include "logo.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "peripheral/pm.h"
#include "string.h"
#include "utils.h"
#define BUFFER_SIZE 64

static int EXIT = 0;
static int load_status = 1;
extern unsigned long dtb_ptr;

void read_command(char* cmd)
{
    int i = 0;
    char c;

    while (1) {
        c = uart_recv();
        uart_send(c);
        if (c == '\n') {
            uart_send('\r');
            cmd[i] = '\0';
            return;
        }
        cmd[i++] = c;
    }
}

void help(void)
{
    uart_send_string(
        "Shell for Raspberry Pi 3B+\n"
        "Available commands:\n"
        "  help      - display this information\n"
        "  hello     - display hello world\n"
        "  reboot    - reboot the system\n"
        "  info      - display system information\n"
        "  logo      - print raspberry pi logo\n"
        "  load      - wait for kernel image and load\n"
        "  boot      - boot the loaded kernel\n");
}

void hello(void)
{
    uart_send_string("Hello, world!\n");
}

void reset(unsigned int tick)
{
    uart_send_string("rebooting...\n");
    EXIT = 1;
    put32(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    put32(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset(void)
{
    uart_send_string("reboot canceled\n");
    EXIT = 0;
    put32(PM_RSTC, PM_PASSWORD | 0);  // cancel reset
    put32(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}

void info(void)
{
    print_board_revision();
    print_arm_memory();
}

void parse_command(char* cmd)
{
    if (!str_cmp(cmd, "help"))
        help();
    else if (!str_cmp(cmd, "hello"))
        hello();
    else if (!str_cmp(cmd, "reboot"))
        reset(10000);
    else if (!str_cmp(cmd, "info"))
        info();
    else if (!str_cmp(cmd, "logo"))
        send_logo();
    else if (!str_cmp(cmd, "load")) {
        load_status = load_img();
        uart_send_string("Done\n");
        switch (load_status) {
        case KERNEL_LOAD_SUCCESS:
            uart_send_string("Kernel loaded\n");
            break;
        case KERNEL_SIZE_ERROR:
            uart_send_string("Kernel size error\n");
            break;
        case KERNEL_LOAD_ERROR:
            uart_send_string("Kernel load error\n");
            break;
        default:
            break;
        }
    } else if (!str_cmp(cmd, "boot")) {
        switch (load_status) {
        case KERNEL_LOAD_SUCCESS:
            ((void (*)(unsigned long))KERNEL_LOAD_ADDR)(dtb_ptr);
            break;
        default:
            uart_send_string("Kernel not loaded\n");
            break;
        }
    } else if (!str_cmp(cmd, "asia_godtone"))
        send_asiagodtone();
    else {
        uart_send_string("Command '");
        uart_send_string(cmd);
        uart_send_string("' not found\n");
    }
}

void shell(void)
{
    uart_send_string("type 'help' to see available commands\n");
    while (!EXIT) {
        char cmd[BUFFER_SIZE];
        uart_send_string("$ ");
        read_command(cmd);
        parse_command(cmd);
    }
}
