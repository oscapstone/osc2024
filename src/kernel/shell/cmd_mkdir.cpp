#include "fs/fs.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_mkdir(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("usage: %s <dir>\n", argv[0]);
    return -1;
  }
  return mkdir(argv[1]);
}
