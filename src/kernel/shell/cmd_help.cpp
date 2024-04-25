#include "io.hpp"
#include "reloc.hpp"
#include "shell/cmd.hpp"

int cmd_help(int /* argc */, char* /* argv */[]) {
  for (int i = 0; i < *reloc(&ncmd); i++) {
    auto cmd = reloc(&cmds[i]);
    kprintf("%s\t%s\n", reloc(cmd->name), reloc(cmd->help));
  }
  return 0;
}
