#include "sched.h"

/**
 * Execute a function in user space.
 * Should create kernel task first, then the kernel task calls this function to create user task.
 */
void do_exec(void (*func)(void))
{
    /* This user task will have sp == (&ustack_pool[current->task_id][USTACK_TOP]) at el0 */
    asm volatile("mov x1, 0              \n\t"
                 "msr spsr_el1, x1       \n\t");
    asm volatile("msr elr_el1, %0" ::"r" (func));
    asm volatile("msr sp_el0, %0" ::"r" (&ustack_pool[current->task_id][USTACK_TOP]));
    asm volatile("eret");

    /* For unknown reason, codes below do not equal codes above. And it should be compiled at `-O2`, or it will not work. */
    // asm volatile ("mov x6, #0x0         \n\t"
    //               "msr spsr_el1, x6     \n\t"
    //               "mov x6, %0           \n\t"
    //               "msr elr_el1, x6      \n\t"
    //               "mov x6, %1           \n\t"
    //               "msr sp_el0, x6       \n\t"
    //               "eret                 \n\t" :: "r" (func), "r" (&ustack_pool[current->task_id][USTACK_TOP]):);
}