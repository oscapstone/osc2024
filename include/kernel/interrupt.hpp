#pragma once

#include "util.hpp"

#define enable_interrupt()  asm("msr DAIFClr, 0xf")
#define disable_interrupt() asm("msr DAIFSet, 0xf")

#define save_DAIF()    auto DAIF = read_sysreg(DAIF)
#define restore_DAIF() write_sysreg(DAIF, DAIF)
