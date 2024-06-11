#include "fs/fs.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_tee(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("usage: %s <file>\n", argv[0]);
    return -1;
  }
  int r = 0;
  auto name = argv[1];
  int fd = open(name, O_RDWR | O_CREAT);
  if (fd < 0) {
    r = -1;
    kprintf("%s: %s: can't open\n", argv[0], name);
  } else {
    for (;;) {
      char c = kgetc();
      if (c == 4)  // ^D
        break;
      write(fd, &c, 1);
      kputc(c);
    }
    r = close(fd);
  }
  return r;
}
