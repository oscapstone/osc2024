#ifndef _P_GPIO_H
#define _P_GPIO_H

#include "peripherals/base.h"

#define GPFSEL1   (PBASE + 0x00200004) // GPIO Function Select 1: Used to set the function of GPIO pins 10-19
#define GPSET0    (PBASE + 0x0020001C) // GPIO Pin Output Set 0: Used to set the output level of GPIO pins 0-31 to high
#define GPCLR0    (PBASE + 0x00200028) // GPIO Pin Output Clear 0: Used to set the output level of GPIO pins 0-31 to low
#define GPPUD     (PBASE + 0x00200094) // GPIO Pin Pull-up/down Enable: Used to enable/disable pull-up/down resistors on GPIO pins
#define GPPUDCLK0 (PBASE + 0x00200098) // GPIO Pin Pull-up/down Enable Clock 0: Used to clock the control signal into the GPIO pads for pins 0-31

#endif /*_P_GPIO_H */