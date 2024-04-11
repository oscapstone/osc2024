#pragma once

#include "mmio.hpp"

#define PERIPHERAL_ENABLE_IRQs_1 ((addr_t)(INTERRUPT_BASE + 0x210))
#define PERIPHERAL_ENABLE_IRQs_2 ((addr_t)(INTERRUPT_BASE + 0x214))

void set_peripheral_irq(bool enable, int bit);
void set_aux_irq(bool enable);
