#include "cmd.h"

#include "mini-uart.h"

void cmd_help(const Cmd_t*);
void cmd_hello(const Cmd_t*);
void cmd_reboot(const Cmd_t*);

const Cmd_t cmds[] = {
    {
        .name = "help",
        .help = "print this help menu",
        .fp = cmd_help,
    },
    {
        .name = "hello",
        .help = "print Hello World!",
        .fp = cmd_hello,
    },
    {
        .name = "reboot",
        .help = "reboot the device",
        .fp = cmd_reboot,
    },
};
const Cmd_t cmds_end[0];

void cmd_help(const Cmd_t* cmd) {
  for (const Cmd_t* cmd = cmds; cmd != cmds_end; cmd++) {
    mini_uart_puts(cmd->name);
    mini_uart_puts("\t");
    mini_uart_puts(cmd->help);
    mini_uart_puts("\n");
  }
}

void cmd_hello(const Cmd_t* cmd) {
  mini_uart_puts("Hello World!\n");
}

void cmd_reboot(const Cmd_t* cmd) {
  // TODO
}
