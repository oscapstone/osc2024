#pragma once

#include "hardware.h"

#define ORDER_FIRST 0
#define ORDER_LAST 255

void enable_interrupt();
void disable_interrupt();
void irq_entry();