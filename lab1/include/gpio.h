#ifndef GPIO_H

#define GPIO_H
#include "base.h"
#define GPFSEL1		(unsigned int*)(BASE+0x00200004)
#define GPSET0		(unsigned int*)(BASE+0x0020001C)
#define GPCLR0		(unsigned int*)(BASE+0x00200028)
#define GPPUD		(unsigned int*)(BASE+0x00200094)
#define GPPUDCLK0	(unsigned int*)(BASE+0x00200098)

#endif
