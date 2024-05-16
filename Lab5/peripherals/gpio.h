#ifndef	_P_GPIO_H
#define	_P_GPIO_H

#include "base.h"

// GPIO14 and GPIO15 are controlled by GPFSEL1(0x3F200004)
#define GPFSEL1     ((volatile unsigned int *)(PBASE+0x00200004))
#define GPSET0      ((volatile unsigned int *)(PBASE+0x0020001C))
#define GPCLR0      ((volatile unsigned int *)(PBASE+0x00200028))
#define GPPUD       ((volatile unsigned int *)(PBASE+0x00200094))
#define GPPUDCLK0   ((volatile unsigned int *)(PBASE+0x00200098))

#endif  /*_P_GPIO_H */
