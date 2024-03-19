#include "shell.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "peripheral/pm.h"
#include "string.h"
#include "utils.h"
#define BUFFER_SIZE 128

int EXIT = 0;

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
        "  help - display this information\n"
        "  hello - display hello world\n"
        "  reboot - reboot the system\n"
        "  info - display system information\n");
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

void print_board_revision(void)
{
    if (!mbox_get_board_revision()) {
        uart_send_string("Unable to get board revision\n");
        return;
    }

    uart_send_string("Board revision: ");
    uart_send_string("0x");
    uart_send_hex(mbox[5]);
    uart_send_string("\n");
}

void print_arm_memory(void)
{
    if (!mbox_get_arm_memory()) {
        uart_send_string("Unable to get arm memory\n");
        return;
    }

    uart_send_string("ARM base address: ");
    uart_send_string("0x");
    uart_send_hex(mbox[5]);
    uart_send_string("\n");

    uart_send_string("ARM memory size: ");
    uart_send_string("0x");
    uart_send_hex(mbox[6]);
    uart_send_string(" bytes\n");
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
        reset(1000);
    else if (!str_cmp(cmd, "info"))
        info();
    else {
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
