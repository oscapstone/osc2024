#pragma once

// D8.2.32 HCR_EL2, Hypervisor Configuration Register (EL2)

#define HCR_RW    (1 << 31)
#define HCR_VALUE HCR_RW

// C4.3.18 SPSR_EL2, Saved Program Status Register (EL2)

#define SPSR_MASK_ALL (0b1111 << 6)
#define SPSR_EL1h     0b0101
#define SPSR_VALUE    (SPSR_MASK_ALL | SPSR_EL1h)
