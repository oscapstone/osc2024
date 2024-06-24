#define MAX_THREAD 32
#define USER_STACK_SIZE 20480
#define KERNEL_STACK_SIZE 2048
#include "stdint.h"
#define READY 1
#define WAIT 2
#define EXIT 3


typedef struct ThreadQueue{
    Thread queue[MAX_THREAD];
    int queue_index;
}

typedef struct Thread{
    int available;
    uint64_t x19; // callee saved registers: the called function will preserve them and restore them before returning
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp; // x29: base pointer for local variable in stack
    uint64_t lr; // x30: store return address
    uint64_t sp; // stack pointer, varys from function calls
    uint16_t* user_stack;
    uint16_t* kernel_stack;
}Thread;