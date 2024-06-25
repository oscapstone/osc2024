#define MAX_THREAD 32
#define USER_STACK_SIZE 20480
#define KERNEL_STACK_SIZE 2048
#include "stdint.h"
// ready to run
#define READY 1
// waiting for IO
#define WAIT 2
// exit but not recycle yet
#define EXIT 3
// thread not exist
#define KILLED 4



typedef struct TaskStateSegment {
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
    uint64_t fp; // x29, frame pointer
    uint64_t lr; // x30, link register
    uint64_t sp;
    uint64_t pc;
}TaskStateSegment;


typedef struct Thread{
    // tss must be placed at first location, for switch_to and get_current to work
    TaskStateSegment tss;
    int available;
    uint16_t* user_stack;
    uint16_t* kernel_stack;
}Thread;



typedef struct ThreadQueue{
    Thread queue[MAX_THREAD];
    int queue_index;
    int current_routine;
}ThreadQueue;


int creat_thread(void (*function_ptr)());
void init_thread();
void exit();
void schedule();
void context_switch(int prev_id, int next_id);
void idle();
void foo();