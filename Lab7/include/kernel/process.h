#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/allocator.h"
#include "kernel/lock.h"
#include "kernel/exception_hdlr.h"
#include "kernel/mem.h"
#include "kernel/syscall.h"
#include "kernel/type.h"
#include "kernel/sys.h"
#include "kernel/timer.h"
#include "kernel/signal.h"
#include "kernel/vfs.h"

#define NR_TASKS 64
#define TASK_RUNNING 1
#define TASK_WAITING 2
#define TASK_ZOMBIE -1
#define PF_KTHREAD   2
#define NR_SIGNALS 64

#define MAX_FD 16

typedef void (*signal_handler_t)(void);

// calee saved registers
typedef struct process_context{
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
    my_uint64_t fp;   //x29, pointed to the bottom of the stack, which is the value of the stack pointer just before the function was called(should be immutable).
    my_uint64_t lr;   //x30, but it's refered as PC in some implementation
    my_uint64_t sp;
}process_context_t;

// this struct must match the format we defined in save_all and load_all
typedef struct task_struct{
    // this is used for saving the context of the process when it's scheduled out
    process_context_t context;
    my_uint64_t sp;
    int status;
    int pid;
    int flag;
    // VFS
    char curr_working_dir[MAX_PATHNAME+1];
    struct file* file_descriptors_table[MAX_FD+1];
    // signal
    signal_handler_t signal_handler[NR_SIGNALS + 1];
    int sigcount[NR_SIGNALS + 1];
    signal_handler_t cur_signal_handler;
    int signal_is_checking;
    process_context_t signal_saved_context;
    // if this is not added, the signal handler might be corrupted. Still looking for reason
    int space[599];
    trap_frame_t tf;
}task_struct_t;

extern task_struct_t *current_task;
extern task_struct_t *PCB[NR_TASKS];
extern int nr_tasks;
// this is the task of kernel shell
#define INIT_TASK { {0,0,0,0,0,0,0,0,0,0,0,0,0}, 0, TASK_RUNNING, -1, PF_KTHREAD,{0}, {0}, {0}, {0}, 0, 0, {0}, {0} }

extern void ret_from_fork(void);
int copy_process(my_uint64_t clone_flags, my_uint64_t fn, my_uint64_t arg, my_uint64_t stack);
int to_el0(my_uint64_t fn);

void process_schedule(void);
void exit_process(void);

void idle_process(void);

void user_process1(unsigned long arg);
void user_process2(void);
void kernel_procsss(void);
void user_process(void);
void file_process(my_uint64_t file_addr);
void fork_test(void);

void pfoo(void);

trap_frame_t *get_task_tf(task_struct_t *task);

#endif