#include "fs/initramfs.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_ls(int /* argc */, char* /* argv */[]) {
  for (auto it : initramfs) {
    kprintf("%c\t%d\t%s\n", "-d"[it->isdir()], it -> filesize(),
            it -> name_ptr());
  }
  return 0;
}
