#ifndef EXCEPTION_HDLR_H
#define EXCEPTION_HDLR_H

#include "kernel/type.h"
// data structure that is used to represent the state of the CPU when an exception or interrupt occurs. 
// When an exception or interrupt is triggered, the OS needs to save the current state of the CPU (registers, program counter, etc.) 
// so that it can be restored later when the exception or interrupt handling is complete.
// including PC, PSR, general-purpose registers 
// and other relevant state information, such as the current execution mode (e.g., user mode vs. kernel mode), the CPU privilege level, and any additional CPU-specific state.

// So do I need to store all the general purpose registers?
// Well, actually we get this trap_frame by using the 'save_all' and 'load_all' in boot.S
// so the order of it should be the same as in 'save_all' as we'll retain it using sp register
typedef struct trap_frame{
    my_uint64_t x0;
    my_uint64_t x1;
    my_uint64_t x2;
    my_uint64_t x3;
    my_uint64_t x4;
    my_uint64_t x5;
    my_uint64_t x6;
    my_uint64_t x7;
    my_uint64_t x8;
    my_uint64_t x9;
    my_uint64_t x10;
    my_uint64_t x11;
    my_uint64_t x12;
    my_uint64_t x13;
    my_uint64_t x14;
    my_uint64_t x15;
    my_uint64_t x16;
    my_uint64_t x17;
    my_uint64_t x18; 
    my_uint64_t x19;
    my_uint64_t x20;
    my_uint64_t x21;
    my_uint64_t x22;
    my_uint64_t x23;
    my_uint64_t x24;
    my_uint64_t x25;
    my_uint64_t x26;
    my_uint64_t x27;
    my_uint64_t x28;
    my_uint64_t fp;           // x29
    my_uint64_t lr;           // x30
    my_uint64_t spsr_el1;     // origial process state
    my_uint64_t elr_el1;      // the address of the instruction that caused the exception(load it when returning to user program from EL1 to EL0)
    my_uint64_t sp_el0;       // stack pointer
}trap_frame_t;

extern int test_NI;
void int_off(void);
void int_on(void);
void c_exception_handler();
void c_core_timer_handler();
void c_write_handler();
void c_recv_handler();
void c_timer_handler();

#endif