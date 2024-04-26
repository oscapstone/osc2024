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


/* architectural feature access control register */
#define CPACR_EL1_FPEN    (1 << 21) | (1 << 20) // don't trap SIMD/FP registers
#define CPACR_EL1_ZEN     (1 << 17) | (1 << 16)  // don't trap SVE instructions
#define CPACR_EL1_VAL     (CPACR_EL1_FPEN | CPACR_EL1_ZEN)

/* exception syndrome register EL1 (ESR_EL1) */
#define ESR_ELx_EC_SHIFT 26
#define ESR_ELx_EC_SVC64 0x15
#define ESR_ELx_EC_DA_LOW 0x24

#define ATTRINDX_NORMAL		0
#define ATTRINDX_DEVICE		1
#define ATTRINDX_COHERENT	2

#define MAIR_VALUENEW (0xFF << ATTRINDX_NORMAL*8	\
	                | 0x04 << ATTRINDX_DEVICE*8	\
	                | 0x00 << ATTRINDX_COHERENT*8)


#define TCR_T0SZ			(64 - 48) 
#define TCR_T1SZ			((64 - 48) << 16)
#define TCR_TG0_4K			(0 << 14)
#define TCR_TG1_4K			(2 << 30)
#define TCR_CONFIG_REGION_48bit (TCR_T0SZ | TCR_T1SZ)
#define TCR_CONFIG_4KB (TCR_TG0_4K |  TCR_TG1_4K)
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

// System registers
#define SCTLR_EL1_WXN		(1 << 19)		// SCTLR_EL1
#define SCTLR_EL1_I		(1 << 12)
#define SCTLR_EL1_C		(1 << 2)
#define SCTLR_EL1_A		(1 << 1)
#define SCTLR_EL1_M		(1 << 0)
