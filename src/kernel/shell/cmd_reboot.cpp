#include "board/pm.hpp"
#include "shell/cmd.hpp"
#include "string.hpp"

int cmd_reboot(int argc, char* argv[]) {
  auto tick = argc < 2 ? 0 : strtol(argv[1]);
  reboot(tick);
  return 0;
}
