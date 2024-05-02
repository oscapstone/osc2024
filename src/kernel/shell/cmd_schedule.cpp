#include "sched.hpp"
#include "shell/cmd.hpp"

int cmd_schedule(int /*argc*/, char* /*argv*/[]) {
  schedule();
  return 0;
}
