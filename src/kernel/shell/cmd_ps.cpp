#include "io.hpp"
#include "shell/cmd.hpp"
#include "thread.hpp"

int cmd_ps(int /*argc*/, char* /*argv*/[]) {
  kprintf("pid\tkthread\n");
  for (auto thread : kthreads)
    kprintf("%d\t%p\n", thread->tid, thread);
  return 0;
}
