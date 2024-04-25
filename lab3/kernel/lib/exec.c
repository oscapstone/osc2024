#include "malloc.h"

void exec(char *program)
{
    char *ustack = (char*)smalloc(2000);

    // goto el0 to run program

    // xzr 0 reg

    // spsr_el1 
    // 0~3 bit 0b0000 : el0t , jump to el0 and use el0 stack
    // 6~9 bit 0b0000 : turn on every interrupt

    // elr_el1 set program start address
    // sp_el0 , stack pointer set to top of program
    // stack往下長的
    asm volatile(
        "msr spsr_el1, xzr\n\t"
        "msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "eret\n\t" ::"r"(program),
        "r"(ustack  + 2000));
}
