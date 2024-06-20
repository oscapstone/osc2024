#ifndef _IRQ_H
#define _IRQ_H

#define EC_SVC 0x15
#define EC_DATAABORT_LEL 0x24 //lower el
#define EC_DATAABORT_SEL 0x25 //same el

typedef struct trapframe
{
    unsigned long x0;
    unsigned long x1;
    unsigned long x2;
    unsigned long x3;
    unsigned long x4;
    unsigned long x5;
    unsigned long x6;
    unsigned long x7;
    unsigned long x8;
    unsigned long x9;
    unsigned long x10;
    unsigned long x11;
    unsigned long x12;
    unsigned long x13;
    unsigned long x14;
    unsigned long x15;
    unsigned long x16;
    unsigned long x17;
    unsigned long x18;
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long x29;
    unsigned long x30;
    unsigned long spsr_el1;
    unsigned long elr_el1;
    unsigned long sp_el0;

} trapframe_t;

typedef struct{
    unsigned int iss : 25, // Instruction specific syndrome
                 il : 1,   // Instruction length bit
                 ec : 6;   // Exception class
} esr_el1_t;

void enable_interrupt_controller( void );
void irq_vector_init( void );
void enable_irq( void );
void disable_irq( void );
void irq_except_handler_timer_c();
void output_trapframe(trapframe_t* tf);



extern int timer_flag;

#endif