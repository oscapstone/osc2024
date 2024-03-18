#include "nanoprintf.hpp"

#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf/nanoprintf.h"

// TODO
#include "util.hpp"
extern "C" void __trunctfsf2() {
  prog_hang();
}
