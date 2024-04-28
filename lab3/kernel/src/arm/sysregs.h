#pragma once


/***
 * In ARMV8 pg.2025
 * SCTLR_EL1, system control register
*/

#define SCTLR_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)
#define SCTLR_E0E_LITTLE_ENDIAN         (0 << 24)
#define SCTLR_I_CACHE_DISABLED          (0 << 12)
#define SCTLR_D_CACHE_DISABLED          (0 << 2)
#define SCTLR_MMU_DISABLED              (0 << 0)
#define SCTLR_MMU_ENABLED               (1 << 0)

#define SCTLR_VALUE_MMU_DISABLED        (SCTLR_RESERVED | \
                                            SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

// pg. 1923
#define HCR_RW                          (1 << 31)
#define HCR_VALUE                       HCR_RW

#define SCR_RESERVED                    (3 << 4)
#define SCR_RW                          (1 << 10)
#define SCR_NS                          (1 << 0)
#define SCR_VALUE                       (SCR_RESERVED | SCR_RW | SCR_NS)


#define SPSR_MASK_ALL                   (7 << 6)
#define SPSR_EL1h                       (5 << 0)
#define SPSR_EL2h                       (9 << 0)
#define SPSR_VALUE                      (SPSR_MASK_ALL | SPSR_EL1h)

/**
 * ARMv8 p. 2038
 * TCR register (Translation Control Register)
*/
#define TCR_T0SZ					((64 - 48) << 0)
#define TCR_T1SZ					((64 - 48) << 16)
#define TCR_TG0_4K					(0 << 14)
#define TCR_TG1_4K					(2 << 30)
// config both TTBR0_EL1 and TTBR1_EL1 memory region
#define TCR_CONFIG_REGION_48bit 	(TCR_T0SZ | TCR_T1SZ)
// set the TG0 and TG1 to 00 and 10 for 4K
#define TCR_CONFIG_4KB 				(TCR_TG0_4K |  TCR_TG1_4K)
#define TCR_CONFIG_DEFAULT 			(TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

// System registers
#define SCTLR_EL1_WXN		(1 << 19)		// SCTLR_EL1
#define SCTLR_EL1_I		(1 << 12)
#define SCTLR_EL1_C		(1 << 2)
#define SCTLR_EL1_A		(1 << 1)
#define SCTLR_EL1_M		(1 << 0)

/**
 * ARMv8 p. 1990
 * MAIR register
 * 64 bit register
 * Provides the memory attribute encodings corresponding to the possible AttrIndx values in a 
 * Long-descriptor format translation table entry for stage 1 translations at EL1.
 * This register is part of the Virtual memory control registers functional group
 * 
 * There are 8 slot, each slot 8 bit
 * for page table descriptor to look the memory policy
 * 
*/
// for Device nGnRnE memory
#define MAIR_DEVICE_nGnRnE      0b00000000
// Normal memory, outer non-cacheable, normal memory, inner non-cacheable
#define MAIR_NORMAL_NOCACHE     0b01000100
// here define the attr0 for device nGnRnE
#define MAIR_IDX_DEVICE_nGnRnE      0
// here define the attr1 for normal non-cacheable memory
#define MAIR_IDX_NORMAL_NOCACHE     1
#define MAIR_DEFAULT_VALUE          ((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))
