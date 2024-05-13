#pragma once

#include "mmio.hpp"

#define PM_PASSWORD 0x5a000000

[[noreturn]] void reboot(int tick = 0);
void reset(int tick);
void cancel_reset();
