#pragma once

#include "base.h"
/**
 * reference to kernel entry load in stack
*/
typedef struct _TRAP_FRAME
{
    U64 sp;       // sp_el0
    U64 blank;
    U64 pc;       // elr_el1
    U64 pstate;   // spsr_el1
    U64 regs[32]; // general purpose regs x0~x30
}TRAP_FRAME;
