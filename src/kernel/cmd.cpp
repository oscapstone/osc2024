#include "cmd.hpp"

#include "board/mailbox.hpp"
#include "board/mini-uart.hpp"
#include "board/pm.hpp"
#include "string.hpp"

int help_idx = 0;
const Cmd cmds[] = {
    {
        ._name = "help",
        ._help = "print this help menu",
        ._fp = cmd_help,
    },
    {
        ._name = "hello",
        ._help = "print Hello World!",
        ._fp = cmd_hello,
    },
    {
        ._name = "hwinfo",
        ._help = "print hardware's infomation",
        ._fp = cmd_hwinfo,
    },
    {
        ._name = "reboot",
        ._help = "reboot the device",
        ._fp = cmd_reboot,
    },
};
constexpr int ncmd = sizeof(cmds) / sizeof(cmds[0]);

void cmd_help() {
  for (int i = 0; i < ncmd; i++) {
    auto cmd = &cmds[i];
    mini_uart_printf("%s\t%s\n", cmd->name(), cmd->help());
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
  reboot();
}

void runcmd(const char* buf, int len) {
  const Cmd* cmd = nullptr;
  for (int i = 0; i < ncmd; i++) {
    auto it = &cmds[i];
    if (!strcmp(buf, it->name())) {
      cmd = it;
      break;
    }
  }
  if (cmd != nullptr) {
    cmd->fp()();
  } else {
    mini_uart_printf("command not found: %s\n", buf);
  }
}
