#include "exec.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"
#include "thread.hpp"

int cmd_run(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("%s: require one argument\n", argv[0]);
    kprintf("usage: %s <program>\n", argv[0]);
    return -1;
  }

  auto ctx = new ExecCtx{argv[1], argv + 1};
  auto th = kthread_create(exec_new_user_prog, (void*)ctx, current_cwd());
  auto tid = th->tid;
  kthread_wait(tid);

  return 0;
}
