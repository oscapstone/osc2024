#include "cmd.hpp"

#include "board/mailbox.hpp"
#include "board/mini-uart.hpp"
#include "board/pm.hpp"

void cmd_help();
void cmd_hello();
void cmd_hwinfo();
void cmd_reboot();

const cmd_t cmds[] = {
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
        .name = "hwinfo",
        .help = "print hardware's infomation",
        .fp = cmd_hwinfo,
    },
    {
        .name = "reboot",
        .help = "reboot the device",
        .fp = cmd_reboot,
    },
};
const cmd_t* cmds_end = (cmd_t*)((char*)cmds + sizeof(cmds));

void cmd_help() {
  for (const cmd_t* cmd = cmds; cmd != cmds_end; cmd++) {
    mini_uart_printf("%s\t%s\n", cmd->name, cmd->help);
  }
}

void cmd_hello() {
  mini_uart_puts("Hello World!\n");
}

void cmd_hwinfo() {
  // it should be 0xa020d3 for rpi3 b+
  mini_uart_printf("Board revision :\t0x%08X\n", get_board_revision());
  mini_uart_printf("ARM memory base:\t0x%08X\n", get_arm_memory(0));
  mini_uart_printf("ARM memory size:\t0x%08X\n", get_arm_memory(1));
}

void cmd_reboot() {
  reset(0x10);
}
