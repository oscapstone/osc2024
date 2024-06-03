#pragma once

#include "base.h"

#include "fs/fs.h"
#include "proc/signal.h"

typedef U64 pid_t;

// /**
//  * A contiguous region of virtual memory information
//  * Just information of the virtual memory in user space not contain page
// */
// typedef struct _VMA_STRUCT {
//     UPTR                    v_start;             // where the virtual addr this page
//     UPTR                    v_end;
//     U32                     flags;
//     struct _VMA_STRUCT*     next;               // next region
// }VMA_STRUCT;

// #define VMA_FLAGS_READ             0x1
// #define VMA_FLAGS_WRITE            0x2
// #define VMA_FLAGS_EXEC             0x4
// #define VMA_FLAGS_GROW_DOWN        0x8
// #define VMA_FLAGS_


#define TASK_MAX_KERNEL_PAGES       20
#define TASK_MAX_USER_PAGES         150

typedef struct _USER_PAGE_INFO {
    pd_t        p_addr;
    U64         v_addr;
    U8          flags;
}USER_PAGE_INFO;
#define TASK_USER_PAGE_INFO_FLAGS_NONE      0x0
#define TASK_USER_PAGE_INFO_FLAGS_READ      0x1
#define TASK_USER_PAGE_INFO_FLAGS_WRITE     0x2
#define TASK_USER_PAGE_INFO_FLAGS_EXEC      0x4
#define TASK_USER_PAGE_INFO_FLAGS_ANON      0x8

typedef struct _MM_STRUCT {
    U64         kernel_pages_count;
    pd_t        kernel_pages[TASK_MAX_KERNEL_PAGES];       // record the level page (table descriptor)
    /**
     * record each block descriptor page
     * first, you need to assign the region this task will allocate in virtual space
    */
    U64             user_pages_count;
    USER_PAGE_INFO  user_pages[TASK_MAX_USER_PAGES];

    //VMA_STRUCT* mmap;               // the information of the virtual addressing

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

typedef struct _FILE_DESCRIPTOR
{
    // fd is the index in task file descriptor table
    FS_FILE* file;
}FILE_DESCRIPTOR;

#define MAX_FILE_DESCRIPTOR 10

// in other word, process
typedef struct _TASK
{
    CPU_REGS cpu_regs;
    pid_t pid;
    U32 status;
    U32 flags;
    int priority;               // the nice value how many time this task can be preempt
    /**
     * If preempt is <= 0 means this task can not be preempted by other task
    */
    int preempt;                // current preempt value
    void* kernel_stack;        // the task process kernel mode stack
    char name[20];              // the task name
    pid_t parent_pid;           // the parent pid
    int exitcode;
    MM_STRUCT mm;

    // SIGNAL
    TASK_SIGNAL signals[SIGNAL_NUM];
    int current_signal;

    // FS
    FS_FILE* program_file;      // the program file that this task run
    FS_VNODE* pwd;              // current working directory
    FILE_DESCRIPTOR file_table[MAX_FILE_DESCRIPTOR];
}TASK;

#define TASK_CPU_REGS_OFFSET    offsetof(cpu_regs, TASK)

// lab said stack 4 page
#define TASK_STACK_PAGE_COUNT   4
#define TASK_STACK_SIZE         (TASK_STACK_PAGE_COUNT * PD_PAGE_SIZE)

#define TASK_STATUS_RUNNING     1
#define TASK_STATUS_WAITING     2
#define TASK_STATUS_DEAD        3
#define TASK_STATUS_SLEEPING    4

#define TASK_FLAGS_NONE         0
#define TASK_FLAGS_ALLOC        0x01            // it is allocated or not
#define TASK_FLAGS_KERNEL       0x04            // it is a kernel task or not

#define TASK_MAX_TASKS          500

#define TASK_SCHE_FREQ          30LL            // in millisecond

typedef struct _TASK_MANAGER {
    U32 count;                                  // current allocated number of task
    U32 running;                                // how many task in running queue
    TASK* running_queue[TASK_MAX_TASKS];        // running queue
    TASK* waiting_list[TASK_MAX_TASKS];         // waiting
    TASK tasks[TASK_MAX_TASKS];
    U32 dead_count;
    TASK* dead_list[TASK_MAX_TASKS];
}TASK_MANAGER;

void task_init();
void task_schedule();
TASK* task_get(int pid);

void task_run_to_el0(TASK* task);
void task_run(TASK* task);
TASK* task_create_kernel(const char* name, U32 flags);             // create kernel process
TASK* task_create_user(const char* name, U32 flags);        // create user process
void task_exit(int exitcode);
/**
 * @param exitcode
 *      0  exit normally
 *      -1 exit with error
 *      -2 exit by killing
*/
int task_kill(pid_t pid, int exitcode);
void task_kill_dead();

/***
 * 
*/
void task_copy_program(TASK* task, void* program_start, size_t program_size);

int task_run_program(FS_VNODE* cwd, TASK* task, const char* program_path);

/**
 * Poorly waiting task for pid
 * @param pid
 *      the task to wait
*/
void task_wait(pid_t pid);

/**
 * Remove the current running task from run list
*/
void task_sleep();

/**
 * Wake up the task of pid
 * @param pid
 *      the pid the task to awake.
*/
void task_awake(pid_t pid);

BOOL task_is_running(TASK* task);
pid_t task_get_pid(TASK* task);

// asm
void task_asm_switch_to(TASK* current, TASK* next);
void task_asm_store_context(CPU_REGS* current);
void task_asm_load_context(CPU_REGS* current);
TASK* task_get_current_el1();