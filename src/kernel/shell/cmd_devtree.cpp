#include "fdt.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"

int cmd_devtree(int argc, char* argv[]) {
  if (argc <= 1) {
    kprintf("%s: require path\n", argv[0]);
    kprintf("usage: %s <path> [depth]\n", argv[0]);
    return -1;
  }

  auto path = argv[1];
  int depth = argc >= 3 ? strtol(argv[2]) : 0;

  int r = 0;
  auto [found, view] = fdt.find(path, print_fdt, depth);
  if (not found) {
    r = -1;
    kprintf("%s: %s: No such device\n", argv[0], path);
  } else if (view.data()) {
    kprintf("%s: ", path);
    kprint_str_or_hex(view);
    kprintf("\n");
  }
  return r;
}
