#ifndef _E_VECTOR_TABLE_H
#define _E_VECTOR_TABLE_H
//set the pt_regs
#define S_FRAME_SIZE 272 
#define S_X0 0 /*reserved 8 bits space, so do the following*/
#define S_X1 8 
#define S_X2 16
#define S_X3 24
#define S_X4 32
#define S_X5 40
#define S_X6 48
#define S_X7 56
#define S_X8 64
#define S_X9 72
#define S_X10 80
#define S_X11 88
#define S_X12 96
#define S_X13 104
#define S_X14 112
#define S_X15 120
#define S_X16 128
#define S_X17 136
#define S_X18 144
#define S_X19 152
#define S_X20 160
#define S_X21 168
#define S_X22 176
#define S_X23 184
#define S_X24 192
#define S_X25 200
#define S_X26 208
#define S_X27 216
#define S_X28 224
#define S_X29 232
#define S_X30 240
#define S_SP 248
#define S_PC 256
#define S_PSTATE 264

// Exception Vector Table No. 
#define SYNC_INVALID_EL1t       0
#define IRQ_INVALID_EL1t        1
#define FIQ_INVALID_EL1t        2
#define ERROR_INVALID_EL1t      3

#define SYNC_INVALID_EL1h       4
#define IRQ_INVALID_EL1h        5
#define FIQ_INVALID_EL1h        6
#define ERROR_INVALID_EL1h      7

#define SYNC_INVALID_EL0_64     8
#define IRQ_INVALID_EL0_64      9
#define FIQ_INVALID_EL0_64      10
#define ERROR_INVALID_EL0_64    11

#define SYNC_INVALID_EL0_32     12
#define IRQ_INVALID_EL0_32      13
#define FIQ_INVALID_EL0_32      14
#define ERROR_INVALID_EL0_32    15

#endif