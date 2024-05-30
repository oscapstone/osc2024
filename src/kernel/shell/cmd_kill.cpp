#include "io.hpp"
#include "shell/cmd.hpp"
#include "signal.hpp"
#include "string.hpp"

int cmd_kill(int argc, char* argv[]) {
  if (argc != 2) {
    kprintf("usage: %s <pid>\n", argv[0]);
    return -1;
  }
  auto pid = strtol(argv[1]);
  kprintf("kill pid = %ld\n", pid);
  signal_kill(pid, SIGKILL);
  return 0;
}
