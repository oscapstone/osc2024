#ifndef PM_H
#define PM_H
#include "peripheral/base.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC (PBASE + 0x0010001c)
#define PM_WDOG (PBASE + 0x00100024)

#endif /* PM_H */
