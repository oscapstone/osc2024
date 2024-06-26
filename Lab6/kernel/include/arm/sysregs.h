#ifndef SYSREGS_H
#define SYSREGS_H


/*******************************************************************************
 * SCTLR_EL1, System Control Register (EL1), Page 2025 of
 * AArch64-Reference-Manual
 *******************************************************************************/
#define SCTLR_RESERVED          ((3 << 28) | (3 << 22) | (1 << 20) | (1 << 11))
#define SCTLR_EE_LITTLE_ENDIAN  (0 << 25)
#define SCTLR_E0E_LITTLE_ENDIAN (0 << 24)
#define SCTLR_I_CACHE_DISABLED  (0 << 12)
#define SCTLR_D_CACHE_DISABLED  (0 << 2)
#define SCTLR_MMU_DISABLED      (0 << 0)
#define SCTLR_MMU_ENABLED       (1 << 0)

#define SCTLR_VALUE                                                      \
    (SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_E0E_LITTLE_ENDIAN | \
     SCTLR_I_CACHE_DISABLED | SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)


/*******************************************************************************
 * HCR_EL2, Hypervisor Configuration Register (EL2), Page 1922 of
 * AArch64-Reference-Manual
 *******************************************************************************/
#define HCR_RW    (1 << 31)
#define HCR_VALUE (HCR_RW)

/*******************************************************************************
 * SCR_EL3, Secure Configuration Register (EL3), Page 1923 of
 * AArch64-Reference-Manual
 *******************************************************************************/
#define SCR_RESERVED (3 << 4)
#define SCR_RW       (1 << 10)
#define SCR_NS       (1 << 0)
#define SCR_VALUE    (SCR_RESERVED | SCR_RW | SCR_NS)


/*******************************************************************************
 * SPSR_EL2, Saved Program Status Register (EL2), Page 283 of
 * AArch64-Reference-Manual
 *******************************************************************************/
#define SPSR_MASK_D (1 << 9)
#define SPSR_MASK_A (1 << 8)
#define SPSR_MASK_I (1 << 7)
#define SPSR_MASK_F (1 << 6)

#define SPSR_MASK_ALL (SPSR_MASK_D | SPSR_MASK_A | SPSR_MASK_I | SPSR_MASK_F)

#define SPSR_EL1h  (5 << 0)
#define SPSR_EL0t  (0)
#define SPSR_VALUE (SPSR_MASK_ALL | SPSR_EL1h)

/*******************************************************************************
 * ESR_ELx, Exception Syndrome Register (ELx), Page 2436 of
 * AArch64-Reference-Manual
 *******************************************************************************/
#define ESR_ELx_EC_SHIFT    26
#define ESR_ELx_EC_SVC64    0x15
#define ESR_ELx_EC_DABT_LOW 0x24

#endif /* SYSREGS_H */
