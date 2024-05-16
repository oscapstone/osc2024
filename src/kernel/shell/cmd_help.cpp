#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_help(int /* argc */, char* /* argv */[]) {
  for (int i = 0; i < ncmd; i++) {
    auto cmd = &cmds[i];
    kprintf("%s\t%s\n", cmd->name, cmd->help);
  }
  return 0;
}
