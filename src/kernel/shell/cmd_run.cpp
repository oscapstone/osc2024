#include "exec.hpp"
#include "fs/initramfs.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

extern char __user_text[];
extern char __user_stack[];

int cmd_run(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("%s: require one argument\n", argv[0]);
    kprintf("usage: %s <program>\n", argv[0]);
    return -1;
  }

  auto name = argv[1];
  auto hdr = initramfs.find(name);
  if (hdr == nullptr) {
    kprintf("%s: %s: No such file or directory\n", argv[0], name);
    return -1;
  }
  if (hdr->isdir()) {
    kprintf("%s: %s: Is a directory\n", argv[0], name);
    return -1;
  }

  auto file = hdr->file();
  memcpy(__user_text, file.data(), file.size());

  exec_user_prog(__user_text, __user_stack);

  return 0;
}
