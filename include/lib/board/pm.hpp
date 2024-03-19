#pragma once

#include "mmio.hpp"

#define PM_PASSWORD 0x5a000000

void reboot();
void reset(int tick);
void cancel_reset();
