#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "reloc.hpp"

int cmd_help(int /* argc */, char* /* argv */[]) {
  for (int i = 0; i < *reloc(&ncmd); i++) {
    auto cmd = reloc(&cmds[i]);
    mini_uart_printf("%s\t%s\n", reloc(cmd->name), reloc(cmd->help));
  }
  return 0;
}
