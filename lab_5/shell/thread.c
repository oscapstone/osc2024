#include "include/thread.h"
#include "include/dynamic_allocator.h"
#include "include/page_allocator.h"

Thread thread_queue[MAX_THREAD];

void init_thread(){
    // create idle thread
}

void creat_thread(void (*function_ptr)()){
    Thread* new_thread = malloc(sizeof(Thread));
    new_thread->user_stack = (uint16_t* )malloc(USER_STACK_SIZE);
    // new_thread->kernel_stack = (uint16_t* )malloc(KERNEL_STACK_SIZE);
    new_thread->x19 = 0x00;
    new_thread->x20 = 0x00;
    new_thread->x21 = 0x00;
    new_thread->x22 = 0x00;
    new_thread->x23 = 0x00;
    new_thread->x24 = 0x00;
    new_thread->x25 = 0x00;
    new_thread->x26 = 0x00;
    new_thread->x27 = 0x00;
    new_thread->x28 = 0x00;
    new_thread->fp = (uint64_t)(new_thread->user_stack+USER_STACK_SIZE); // x29: base pointer for local variable in stack
    new_thread->lr = 0x00; // x30: store return address
    new_thread->sp = new_thread->fp; // stack pointer, varys from function calls
    
}