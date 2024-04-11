#pragma once    //確保只到include一次

// ***************************************
// SCTLR_EL1, System Control Register (EL1), Page 202 of AArch64-Reference-Manual.
// https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/SCTLR-EL1--System-Control-Register--EL1-
// ***************************************

#define SCTLR_RESERVED              ((3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)) //29.28.23.22.20.11
#define SCTLR_EE_LITTLE_ENDIAN      (0 << 25)   //Explicit data accesses at EL1, and stage 1 translation table walks in the EL1&0 translation regime are little-endian.
#define SCTLR_EOE_LITTLE_ENDIAN     (0 << 24)   //Explicit data accesses at EL0 are little-endian
#define SCTLR_I_CACHE_DISABLED      (0 << 12)   //All instruction access to Stage 1 Normal memory from EL0 and EL1 are Stage 1 Non-cacheable.
#define SCTLR_D_CACHE_DISABLED      (0 << 2)    //All data access to Stage 1 Normal memory from EL0 and EL1, and all Normal memory accesses from unified cache to the EL1&0 Stage 1 translation tables, are treated as Stage 1 Non-cacheable.
#define SCTLR_MMU_DISABLED          (0 << 0)    //EL1&0 stage 1 address translation disabled.
#define SCTLR_MMU_ENABLED           (1 << 0)    //EL1&0 stage 1 address translation enabled.
//處理器的記憶管理單元（MMU）中的第一階段地址翻譯，用於將虛擬地址轉換為物理地址

#define SCTLR_VALUE_MMU_DISABLED    (SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

// ***************************************
// HCR_EL2, Hypervisor Configuration Register (EL2), Page 2487 of AArch64-Reference-Manual.
// https://developer.arm.com/docs/ddi0595/b/aarch64-system-registers/hcr_el2
// ***************************************

#define HCR_RW                      (1 << 31) // The Execution state for EL1 is AArch64
#define HCR_EL2_VALUE               HCR_RW

// ***************************************
// SCR_EL3, Secure Configuration Register (EL3), No EL012
//https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch64-register-descriptions/Secure-Configuration-Register
// ***************************************

#define SCR_RESERVED                (3 << 4)    //Reserved, res1.
#define SCR_RW                      (1 << 10)   //The next lower level is AArch64.
#define SCR_NS                      (1 << 0)    //EL0 and EL1 are in Non-secure state, memory accesses from those exception levels cannot access Secure memory.
#define SCR_VALUE                   (SCR_RESERVED | SCR_RW | SCR_NS)

// ***************************************
// SPSR_EL2, Saved Program Status Register (EL2)
// https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-
// ***************************************

#define SPSR_AARCH64                (0 << 4)    //AArch64 execution state.
//When the processor takes an exception to an AArch64 execution state, the PSTATE interrupt masks (PSTATE.DAIF) are set automatically. 
//DAIF stands for debug, abort (SError), IRQ, and FIQ. The DAIF field is 4 bits, with each bit corresponding to one of the mentioned exception types. 
//By writing a 1 to a bit in the field, we mask or ignore the exception type. 
#define SPSR_MASK_D                 (1 << 9)
#define SPSR_MASK_A                 (1 << 8)
#define SPSR_MASK_I                 (1 << 7)
#define SPSR_MASK_F                 (1 << 6)
// Exception level and selected Stack Pointer. from which level we trap in to el2
#define SPSR_EL0t                   (0b0000 << 0)   //EL0.
#define SPSR_EL1t                   (0b0100 << 0)   //EL1 with SP_EL0 (EL1t)
#define SPSR_EL1h                   (0b0101 << 0)   //EL1 with SP_EL1 (EL1h
#define SPSR_EL2t                   (0b1000 << 0)   //EL2 with SP_EL0 (EL2t).
#define SPSR_EL2h                   (0b1001 << 0)   //EL2 with SP_EL2 (EL2h).
#define SPSR_EL2_VALUE              (SPSR_MASK_D | SPSR_MASK_A | SPSR_MASK_I | SPSR_MASK_F | SPSR_EL1h)
#define SPSR_EL1_VALUE              (SPSR_MASK_D | SPSR_MASK_A | SPSR_MASK_F | SPSR_EL1h)
