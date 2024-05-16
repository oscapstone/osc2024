#pragma once

#ifndef MASK
#define MASK(bits) ((1ULL << bits) - 1)
#endif

// D8.2.32 HCR_EL2, Hypervisor Configuration Register (EL2)

#define HCR_RW    (1 << 31)
#define HCR_VALUE HCR_RW

// C4.3.18 SPSR_EL2, Saved Program Status Register (EL2)

#define SPSR_MASK_ALL (0b1111 << 6)
#define SPSR_EL1h     0b0101
#define SPSR_VALUE    (SPSR_MASK_ALL | SPSR_EL1h)

// ESR

#define ESR_ELx_EC_SHIFT (26)
#define ESR_ELx_EC_WIDTH (6)
#define ESR_ELx_EC_MASK  (MASK(6) << ESR_ELx_EC_SHIFT)
#define ESR_ELx_EC(esr)  (((esr) & ESR_ELx_EC_MASK) >> ESR_ELx_EC_SHIFT)

#define ESR_ELx_IL_SHIFT (25)
#define ESR_ELx_IL       (MASK(1) << ESR_ELx_IL_SHIFT)
#define ESR_ELx_ISS_MASK (MASK(25))
#define ESR_ELx_ISS(esr) ((esr) & ESR_ELx_ISS_MASK)

#define ESR_ELx_EC_SVC64    (0b010101)
#define ESR_ELx_EC_IABT_LOW (0b100000)
#define ESR_ELx_EC_IABT_CUR (0b100001)
#define ESR_ELx_EC_PC_ALIGN (0b100010)
#define ESR_ELx_EC_DABT_LOW (0b100100)
#define ESR_ELx_EC_DABT_CUR (0b100101)
#define ESR_ELx_EC_SP_ALIGN (0b100110)
