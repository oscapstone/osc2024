#include "fs/fs.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_touch(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("usage: %s <file>\n", argv[0]);
    return -1;
  }
  int fd = open(argv[1], O_CREAT);
  if (fd < 0)
    return fd;
  return close(fd);
}
