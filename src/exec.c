#include "sched.h"



/* Execute `func` function at user mode. */
void do_exec(void (*func)(void))
{
    /* This user task will have sp == (&ustack_pool[current->task_id][USTACK_TOP]) at el0 */
    asm volatile("msr spsr_el1, %0" ::"r" (0x0));
    asm volatile("msr elr_el1, %0" ::"r" (func));
    asm volatile("msr sp_el0, %0" ::"r" (&ustack_pool[current->task_id][USTACK_TOP]));
    asm volatile("eret");

    /* For unknown reason, codes below do not equal codes above. And it should be compiled at `-O2`, or it will not work. */
    // asm volatile ("mov x0, #0x0         \n\t"
    //               "msr spsr_el1, x0     \n\t"
    //               "mov x0, %0           \n\t"
    //               "msr elr_el1, x0      \n\t"
    //               "mov x0, %1           \n\t"
    //               "msr sp_el0, x0       \n\t"
    //               "eret                 \n\t" :: "r" (func), "r" (&ustack_pool[current->task_id][USTACK_TOP]):);
}