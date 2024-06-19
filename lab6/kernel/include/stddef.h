#ifndef STDDEF_H
#define STDDEF_H

#define size_t unsigned long

#ifndef __ASSEMBLER__

typedef struct{
    unsigned int iss : 25, // Instruction specific syndrome
                 il : 1,   // Instruction length bit
                 ec : 6;   // Exception class
} esr_el1_t;

#endif //__ASSEMBLER__

#endif