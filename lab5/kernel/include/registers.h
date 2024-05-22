#ifndef REGISTERS_H
#define REGISTERS_H

// CPACR_EL1, Architectural Feature Access Control Register
#define CPACR_EL1_FPEN      (0b11 << 20)
#define CPACR_EL1_VALUE     (CPACR_EL1_FPEN)
#define IRQS1  ((volatile unsigned int*)(0x3f00b210))

#define STR(x) #x
#define XSTR(s) STR(s)

#endif