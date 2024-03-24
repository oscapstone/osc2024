#include "shell.h"
#include "cpio.h"
#include "def.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "peripheral/pm.h"
#include "string.h"
#include "utils.h"
#define BUFFER_SIZE 128

static int EXIT = 0;

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
        "  help   - display this information\n"
        "  hello  - display hello world\n"
        "  reboot - reboot the system\n"
        "  info   - display system information\n"
        "  ls     - list files in the initramfs\n"
        "  cat    - display file content\n");
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
    if (!strcmp(cmd, "help"))
        help();
    else if (!strcmp(cmd, "hello"))
        hello();
    else if (!strcmp(cmd, "reboot"))
        reset(100000);
    else if (!strcmp(cmd, "info"))
        info();
    else if (!strcmp(cmd, "ls"))
        ls();
    else if (!strcmp(strtok(cmd, " "), "cat")) {
        char* filename = strtok(NULL, " ");
        if (!filename)
            uart_send_string("Usage: cat <filename>\n");
        else
            cat(filename);
    } else {
        uart_send_string("Command '");
        uart_send_string(cmd);
        uart_send_string("' not found\n");
    }
}

void shell(void)
{
    while (!EXIT) {
        char cmd[BUFFER_SIZE];
        uart_send_string("$ ");
        read_command(cmd);
        parse_command(cmd);
    }
}
