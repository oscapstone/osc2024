#pragma once

#include "util.hpp"

#define enable_interrupt()  asm("msr DAIFClr, 0xf")
#define disable_interrupt() asm("msr DAIFSet, 0xf")

#define save_DAIF()    auto DAIF = read_sysreg(DAIF)
#define restore_DAIF() write_sysreg(DAIF, DAIF)

#define save_DAIF_disable_interrupt() \
  save_DAIF();                        \
  disable_interrupt()

class SaveDAIF {
  uint64_t DAIF;

 public:
  SaveDAIF() : DAIF{read_sysreg(DAIF)} {}
  ~SaveDAIF() {
    write_sysreg(DAIF, DAIF);
  }
};
