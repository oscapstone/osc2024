#include "fs/files.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_ls(int argc, char* argv[]) {
  auto vnode = current_cwd();
  if (argc > 1) {
    vnode = vfs_lookup(argv[1]);
    if (vnode == nullptr) {
      kprintf("%s: error\n", argv[0]);
      return -1;
    }
  }
  for (auto& it : vnode->childs()) {
    kprintf("%c\t%ld\t%s\n", "-d"[it.node->isDir()], it.node->size(), it.name);
  }
  return 0;
}
