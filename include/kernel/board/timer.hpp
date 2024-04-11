#pragma once

#include "util.hpp"

#define CORE0_TIMER_IRQ_CTRL ((addr_t)0x40000040)

void set_core0_timer_irq_ctrl(bool enable, int bit);
