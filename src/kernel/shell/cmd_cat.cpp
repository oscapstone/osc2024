#include "fs/fs.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_cat(int argc, char* argv[]) {
  int r = 0;
  if (argc == 1) {
    for (;;) {
      char c = kgetc();
      if (c == 4)  // ^D
        break;
      kputc(c);
    }
  } else {
    for (int i = 1; i < argc; i++) {
      auto name = argv[i];
      auto f = vfs_lookup(name);
      if (f == nullptr) {
        r = -1;
        kprintf("%s: %s: No such file or directory\n", argv[0], name);
      } else if (f->isDir()) {
        r = -1;
        kprintf("%s: %s: Is a directory\n", argv[0], name);
      } else {
        File* file;
        if (vfs_open(name, O_RDONLY, file) < 0) {
          r = -1;
          kprintf("%s: %s: can't open\n", argv[0], name);
        } else {
          char buf[128];
          int r;
          while ((r = file->read(buf, sizeof(buf))) > 0)
            kwrite(buf, r);
        }
      }
    }
  }
  return r;
}
