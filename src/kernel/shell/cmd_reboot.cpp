#include "board/pm.hpp"
#include "shell/cmd.hpp"

int cmd_reboot(int /* argc */, char* /* argv */[]) {
  reboot();
  return 0;
}
