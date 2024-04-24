#include "board/mailbox.hpp"
#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_hwinfo(int /* argc */, char* /* argv */[]) {
  // it should be 0xa020d3 for rpi3 b+
  kprintf("Board revision :\t0x%08X\n", get_board_revision());
  kprintf("ARM memory base:\t0x%08X\n", get_arm_memory(0));
  kprintf("ARM memory size:\t0x%08X\n", get_arm_memory(1));
  return 0;
}
