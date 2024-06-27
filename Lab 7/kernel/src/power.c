#include "mmio.h"
#include "power.h"


#define PM_RSTC         ((volatile unsigned int*)(MMIO_BASE+0x0010001c))
#define PM_RSTS         ((volatile unsigned int*)(MMIO_BASE+0x00100020))
#define PM_WDOG         ((volatile unsigned int*)(MMIO_BASE+0x00100024))

#define PM_MAGIC        0x5a000000
#define PM_RSTC_FULLRST 0x00000020


void 
power_reset(uint32_t ticks)
{
    *PM_RSTC = (PM_MAGIC | PM_RSTC_FULLRST);            // full reset
    *PM_WDOG = (PM_MAGIC | ticks);                      // number of watchdog ticks 
}


void
power_reset_cancel() 
{
    *PM_RSTC = (PM_MAGIC | 0);                          // cancel reset
    *PM_WDOG = (PM_MAGIC | 0);                          // number of watchdog ticks 
}