#ifndef P_TIMER_H
#define P_TIMER_H

#include "mm.h"

#define CORE_TIMER_BASE (VA_START + 0x40000000)
/*
 * Core Timers interrupt control, Page 13 of QA7_rev3.4 data sheet
 */
#define CORE0_TIMER_IRQ_CTRL   (CORE_TIMER_BASE + 0x40)
#define nCNTPNSIRQ_IRQ_ENABLE  (1 << 1)
#define CORE_TIMER_DISABLE_ALL 0


/*
 * Core interrupt source, Page 16 of QA7_rev3.4 data sheet
 */
#define CORE0_IRQ_SOURCE (CORE_TIMER_BASE + 0x60)
#define CNTPNSIRQ        (1 << 1)


/*
 * CNTP_CTL_EL0, Counter-timer Physical Timer Control register, Page 2945 of
 * AArch64-Reference-Manual
 */
#define CNTP_CTL_EL0_ENABLE  1
#define CNTP_CTL_EL0_DISABLE 0


#endif /* P_TIMER_H */
