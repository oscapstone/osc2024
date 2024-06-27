#include "thread.h"
#include "asm.h"
#include "frame.h"
#include "sched.h"
#include "uart.h"
#include "delay.h"
#include "memory.h"
#include "exception.h"


void
thread_exit(int32_t status)
{    
    scheduler_kill(current_task(), status);
}


int32_t
thread_create_shell(uint64_t fn)
{
    task_ptr p = task_create(fn, 0);
    if (!p) return -1;
    uart_printf("[DEBUG] thread_create pid: %d (0x%x)\n", p->pid, p);
    scheduler_add_shell(p);
    return 0;
}


task_ptr
thread_create(uint64_t fn, uint64_t arg)
{
    task_ptr p = task_create(fn, arg);
    if (!p) return 0;
    uart_printf("[DEBUG] thread_create pid: %d (0x%x)\n", p->pid, p);
    scheduler_add(p);
    return p;
}


int32_t
thread_create_user(uint64_t fn, uint64_t program, uint32_t size)
{
    uart_printf("[DEBUG] thread_create_user - ");
 
    task_ptr p;
    if (size) {
        // code duplicate
        uint32_t count = (uint32_t) memory_padding((byteptr_t) (size | 0l), FRAME_SIZE_ORDER) >> FRAME_SIZE_ORDER;
        byteptr_t user_prog = frame_alloc(count);
        uart_printf("frame_count: %d, user_prog: 0x%x", count, user_prog);
        if (!user_prog) { return -1; }
        memory_copy(user_prog, (byteptr_t) program, size);
        uart_printf("(from program: 0x%x), size: %d, ", program, size);

        // build a new task
        p = task_create(fn, (uint64_t) user_prog);  // fn = run_user_process 
        p->user_prog = user_prog;
        p->user_prog_size = size;
        p->frame_count = count;
        p->foreground = 1;
    }
    else { 
        // build a new task
        p = task_create(fn, (uint64_t) program);
        p->user_prog = 0;
        p->user_prog_size = 0;
        p->frame_count = 0;
        p->foreground = 1;
    }
 
    uart_printf("pid: %d (%x)\n", p->pid, p);
    scheduler_add(p);
    return p->pid;
}


int32_t
thread_replace_user(uint64_t fn, uint64_t program, uint32_t size)
{
    task_ptr p = current_task();
    uint32_t foreground = p->foreground;

    uart_printf("[DEBUG] thread_replace_user - ");

    if (size) {
        // code duplicate
        uint32_t count = (uint32_t) memory_padding((byteptr_t) (size | 0l), FRAME_SIZE_ORDER) >> FRAME_SIZE_ORDER;
        byteptr_t user_prog = frame_alloc(count);
        uart_printf("frame_count: %d, user_prog: 0x%x", count, user_prog);
        if (!user_prog) { return -1; }
        memory_copy(user_prog, (byteptr_t) program, size);
        uart_printf("(from program: 0x%x), size: %d, ", program, size);

        // reset the current task
        // task_reset(p, fn, (uint64_t) user_prog); // x19 = fn (e.g, run_user_process, it will call to_user(x20))
        p->user_prog = user_prog;
        p->user_prog_size = size;
        p->frame_count = count;
        p->foreground = foreground;
    } 
    
    else {
        p->user_prog = 0;
        p->user_prog_size = 0;
        p->frame_count = 0;
        p->foreground = foreground;
    } 

    uart_printf("pid: %d (%x)\n", p->pid, p);
    return p->pid;
} 


int32_t
thread_fork()
{
    task_ptr child = (task_ptr) task_fork(current_task());
    scheduler_add(child);
 
    return child->pid;
}


#include "asm.h"


static void 
foo()
{
    for (int i = 0; i < 5; ++i) {
        uart_printf("> thread id: %d i=%d\n",  current_task()->pid, i);
        uint64_t freq = asm_read_sysregister(cntfrq_el0);
        delay_cycles(freq >> 5);
    }
    thread_exit(0);
}


void 
thread_demo()
{
    for (int i = 0; i < 4; ++i) {
        uart_printf("Thread_test i: %d, ", i);
        thread_create((uint64_t) &foo, 0);
    }
}
