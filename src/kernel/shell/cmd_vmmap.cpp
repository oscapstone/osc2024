#include "io.hpp"
#include "shell/cmd.hpp"
#include "thread.hpp"

int cmd_vmmap(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("usage: %s <pid>\n", argv[0]);
    return -1;
  }
  auto pid = strtol(argv[1]);
  auto th = find_thread_by_tid(pid);
  if (not th) {
    kprintf("%s: couldn't find pid %ld\n", argv[0], pid);
    return -1;
  }
  th->vmm.vma_print();
  return 0;
}
