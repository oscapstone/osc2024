#include "board/mini-uart.hpp"
#include "cmd.hpp"

int cmd_help(int /* argc */, char* /* argv */[]) {
  for (int i = 0; i < ncmd; i++) {
    auto cmd = &cmds[i];
    mini_uart_printf("%s\t%s\n", cmd->name, cmd->help);
  }
  return 0;
}
