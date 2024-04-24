#include "io.hpp"
#include "shell/cmd.hpp"

int cmd_hello(int /* argc */, char* /* argv */[]) {
  kputs("Hello World!\n");
  return 0;
}
