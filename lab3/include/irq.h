#pragma once

#include "hardware.h"

#define PRIORITY_HIGH 0
#define PRIORITY_LOW 255

void enable_interrupt();
void disable_interrupt();
void irq_entry();