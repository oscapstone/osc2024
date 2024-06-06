#include "fs/fs.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_pwd(int argc, char* argv[]) {
  list<const char*> names{};
  auto vnode = current_cwd();
  while (vnode->parent() != vnode) {
    names.push_front(vnode->parent()->lookup(vnode));
    vnode = vnode->parent();
  }
  auto basename = names.pop_back();
  for (auto x : names)
    kprintf("/%s", x);
  kprintf("/%s\n", basename);
  return 0;
}
