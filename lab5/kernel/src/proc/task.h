#pragma once

#include "base.h"

typedef U64 pid_t;

typedef struct _VMA_PAGE_INFO {
    U64                         v_addr;
    U64                         p_addr;                     // this page physical address
    struct _VMA_PAGE_INFO*      next;
}VMA_PAGE_INFO;

/**
 * A contiguous region of virtual memory information
*/
typedef struct _VMA_STRUCT {
    UPTR                    v_start;             // where the virtual addr this page
    UPTR                    v_end;
    U32                     flags;
    U32                     page_count;         // number of page in this region
    struct _VMA_PAGE_INFO*  pages;           // the page info (block descriptor)
    struct _VMA_STRUCT*     next;               // next region
}VMA_STRUCT;

#define VMA_FLAGS_READ             0x1
#define VMA_FLAGS_WRITE            0x2
#define VMA_FLAGS_EXEC             0x4
#define VMA_FLAGS_GROW_DOWN        0x8


#define TASK_MAX_KERNEL_PAGES       20
#define TASK_MAX_USER_PAGES         100

typedef struct _USER_PAGE_INFO {
    pd_t        p_addr;
    U64         v_addr;
    U8          flags;
}USER_PAGE_INFO;
#define TASK_USER_PAGE_INFO_FLAGS_NONE      0x0
#define TASK_USER_PAGE_INFO_FLAGS_READ      0x1
#define TASK_USER_PAGE_INFO_FLAGS_WRITE     0x2
#define TASK_USER_PAGE_INFO_FLAGS_EXEC      0x4

typedef struct _MM_STRUCT {
    U64         kernel_pages_count;
    pd_t        kernel_pages[TASK_MAX_KERNEL_PAGES];       // record the level page (table descriptor)
    /**
     * record each block descriptor page
     * first, you need to assign the region this task will allocate in virtual space
    */
    U64             user_pages_count;
    USER_PAGE_INFO  user_pages[TASK_MAX_USER_PAGES];

}MM_STRUCT;


typedef struct _CPU_REGS
{
    U64 x19, x20;
    U64 x21, x22;
    U64 x23, x24;
    U64 x25, x26;
    U64 x27, x28;

    // fp,=x29
    // lr,=x30, link register, use for jump to current task pc too
    U64 fp, lr;             

    U64 sp, pgd;
}CPU_REGS;

// in other word, process
typedef struct _TASK
{
    CPU_REGS cpu_regs;
    pid_t pid;
    U32 flags;
    int priority;               // the nice value how many time this task can be preempt
    /**
     * If preempt is <= 0 means this task can not be preempted by other task
    */
    int preempt;                // current preempt value
    void* kernel_stack;        // the task process kernel mode stack
    void* user_stack;
    char name[20];              // the task name
    pid_t parent_pid;           // the parent pid
    MM_STRUCT mm;
}TASK;

#define TASK_CPU_REGS_OFFSET    offsetof(cpu_regs, TASK)

// lab said stack 4 page
#define TASK_STACK_PAGE_COUNT   4
#define TASK_STACK_SIZE         (TASK_STACK_PAGE_COUNT * PD_PAGE_SIZE)

#define TASK_FLAGS_ALLOC        0x01            // it is allocated or not
#define TASK_FLAGS_RUNNING      0x02            // it is running or not
#define TASK_FLAGS_KERNEL       0x04            // it is a kernel task or not
#define TASK_FLAGS_DEAD         0x08

#define TASK_MAX_TASKS          500

typedef struct _TASK_MANAGER {
    U32 count;                                  // current allocated number of task
    U32 running;                                // how many task in running queue
    TASK* running_queue[TASK_MAX_TASKS];        // running queue
    TASK tasks[TASK_MAX_TASKS];
    U32 dead_count;
    TASK* dead_list[TASK_MAX_TASKS];
}TASK_MANAGER;

void task_init();
void task_schedule();

void task_run_to_el0(TASK* task);
void task_run(TASK* task);
TASK* task_create(const char* name, U32 flags);             // create kernel process
TASK* task_create_user(const char* name, U32 flags);        // create user process
void task_exit();
void task_kill(TASK* task);
void task_kill_dead();

/***
 * 
*/
void task_copy_program(TASK* task, void* program_start, size_t program_size);

// asm
void task_asm_switch_to(TASK* current, TASK* next);
void task_asm_store_context(TASK* current);
TASK* task_get_current_el1();