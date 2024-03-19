#include "board/mini-uart.hpp"
#include "cmd.hpp"

void cmd_help() {
  for (int i = 0; i < ncmd; i++) {
    auto cmd = &cmds[i];
    mini_uart_printf("%s\t%s\n", cmd->name(), cmd->help());
  }
}
