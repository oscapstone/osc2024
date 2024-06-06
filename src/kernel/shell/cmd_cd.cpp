#include "fs/files.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_cd(int argc, char* argv[]) {
  const char* path = argc > 1 ? argv[1] : "/";
  if (chdir(path) < 0) {
    kprintf("%s: error\n", argv[0]);
    return -1;
  }
  return 0;
}
