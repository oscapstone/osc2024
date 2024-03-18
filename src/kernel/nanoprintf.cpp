#include "nanoprintf.h"

#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf/nanoprintf.h"

// TODO
#include "util.h"
extern "C" void __trunctfsf2() {
  prog_hang();
}
