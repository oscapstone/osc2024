#ifndef _THREAD_H
#define _THREAD_H

#include "util.h"
#include "type.h"
#include "mem.h"
#include "mini_uart.h"

#define FREE    0
#define READY   1
#define RUNNING 2
#define DEAD    3

#define MAX_TID 32768
#define USTACK_SIZE 0x1000
#define KSTACK_SIZE 0x1000

typedef struct thread_ctx {
    uint64_t x19;
    uint64_t x20;
    uint64_t x21; 
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28; 
    uint64_t fp;
    uint64_t lr; 
    uint64_t sp;
} thread_ctx_t;

typedef struct thread {
    uint32_t id;
    uint32_t status;
    thread_ctx_t ctx;
    uint8_t* user_sp;
    uint8_t* kernel_sp;
    uint8_t* prog;
    uint32_t prog_size;
    struct thread* prev;    
    struct thread* next;
} thread_t;

typedef struct trap_frame {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
    uint64_t x8;
    uint64_t x9;
    uint64_t x10;
    uint64_t x11;
    uint64_t x12;
    uint64_t x13;
    uint64_t x14;
    uint64_t x15;
    uint64_t x16;
    uint64_t x17;
    uint64_t x18;
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x29;
    uint64_t x30;
    uint64_t spsr_el1;
    uint64_t elr_el1;
    uint64_t sp_el0;
} trap_frame_t;

void init_thread_arr();
thread_t* _thread_create(void (*)(void));
void child_fork_ret();

#endif