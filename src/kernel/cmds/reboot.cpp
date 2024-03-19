#include "board/pm.hpp"
#include "cmd.hpp"

int cmd_reboot(int /* argc */, char* /* argv */[]) {
  reboot();
  return 0;
}
