#ifndef __SYSREGS_H__
#define __SYSREGS_H__
// EL0


// EL1
// SCTLR_EL1, System Control Register (EL1)
#define SCTLR_MMU_DISABLED (0 << 0) //關閉MMU映射
#define SCTLR_EE_LITTLE_ENDIAN  ( 0 << 25) //設定EL1 下資料存取的大小端，0 : 小端
#define SCTLR_EOE_LITTLE_ENDIAN ( 0 << 24) //設定EL0 下資料存取的大小端。0 : 小端
#define SCTLR_VALUE_MMU_DISABLED (SCTLR_MMU_DISABLED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_EOE_LITTLE_ENDIAN)
// SPSR_EL1, Saved Program Status Register (EL1)
#define SPSR_EL1_MASK       (0b1111 << 6) 
#define SPSR_EL1_EL0        (0b0000 << 0)    // EL1 to EL0
#define SPSR_EL1      (SPSR_EL1_MASK | SPSR_EL1_EL0)

// CPACR_EL1, Architectural Feature Access Control Register
// #define CPACR_EL1_FPEN      (0b11 << 20)
// #define CPACR_EL1_VALUE     (CPACR_EL1_FPEN)


// EL2
// HCR_EL2, Hypervisor Configuration Register
#define HCR_RW   ( 1 << 31) //HCR_RW_AARCH64 紀錄Exception發生後，EL1要在哪個狀態執行。 1: in AArch64

// SPSR_EL2, Saved Program Status Register (EL2)
#define SPSR_EL2_MASK_ALL   (0b1111 << 6)
#define SPSR_EL2_EL1h       (0b0101 << 0)  // EL2 to EL1 

#define SPSR_EL2      (SPSR_EL2_MASK_ALL | SPSR_EL2_EL1h) // a.k.a 0x3c5 (0011 1100 0101)

//Timer in EL0

#define CNTP_CTL_ENABLE (0b0001 << 0)

#endif