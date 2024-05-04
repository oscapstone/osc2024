#include "exec.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_exec(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("%s: require one argument\n", argv[0]);
    kprintf("usage: %s <program>\n", argv[0]);
    return -1;
  }

  auto name = argv[1];
  return exec(name, nullptr);
}
