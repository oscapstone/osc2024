#ifndef __GPIO_H__
#define __GPIO_H__

#include "base.h"
// #define MMU_BASE 0x3f000000 // corresponding to bus address 0x7E000000

#define GPFSEL1   (volatile unsigned int*)(MMU_BASE+0x00200004) // Set GPIO14 and GPIO15, should choose FSEL1(pin10~19)
#define GPPUD     (volatile unsigned int*)(MMU_BASE+0x00200094)
#define GPPUDCLK0 (volatile unsigned int*)(MMU_BASE+0x00200098)

#endif