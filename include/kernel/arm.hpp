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

// D8.2.82 TCR_EL1, Translation Control Register(EL1)

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

// D5.5.2 Memory region attributes
// D8.2.61 MAIR_EL1, Memory Attribute Indirection Register (EL1)

#define MAIR_DEVICE_nGnRnE      0b00000000
#define MAIR_NORMAL_NOCACHE     0b01000100
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE      0b11
#define PD_BLOCK      0b01
#define PD_ACCESS     (1 << 10)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
