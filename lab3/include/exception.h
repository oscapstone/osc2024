#ifndef __EXCEPTION_H
#define __EXCEPTION_H

// LAB3-1
#ifndef __ASSEMBLER__
void el2_to_el1();
void branch_el1_to_el0(char *addr, char *sp);
#endif

#endif