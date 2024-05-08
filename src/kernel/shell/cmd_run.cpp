#include "exec.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"
#include "thread.hpp"

int cmd_run(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("%s: require one argument\n", argv[0]);
    kprintf("usage: %s <program>\n", argv[0]);
    return -1;
  }

  // TODO: handle argv
  auto name = strdup(argv[1]);
  kthread_create(exec_new_user_prog, (void*)name);
  return 0;
}
