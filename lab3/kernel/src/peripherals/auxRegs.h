#pragma once

#include "base.h"
#include "bcm2837base.h"

// on BCM2837 arm peripheral pg.8
struct AuxRegs {
    REG32 irq_status;
    REG32 enables;
    REG32 reserved[14];
    REG32 mu_io;            // mini uart i/o data
    REG32 mu_ier;           // mini uart interrupt enable
    REG32 mu_iir;           // mini uart interrupt identify
    REG32 mu_lcr;           // mini uart line control
    REG32 mu_mcr;           // mini uart modem control
    REG32 mu_lsr;           // mini uart line status
    REG32 mu_msr;           // mini uart modem status
    REG32 mu_scratch;       // mini uart scratch
    REG32 mu_control;       // mini uart extra control
    REG32 mu_status;        // mini uart extra status
    REG32 mu_baud_rate;     // mini uart baudrate
};

// 0x7e215000 is bus address
// 0x7e215000 - (bus offset: 0x7e000000) + (physical offset: 0x3f000000)
#define REGS_AUX ((struct AuxRegs *) (PBASE + 0x00215000))

