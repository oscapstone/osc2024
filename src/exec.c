#include "sched.h"



/* Execute `func` function at user mode */
void do_exec(void (*func)(void))
{
    /* This user task will have sp == (&ustack_pool[current->task_id][USTACK_TOP]) at el0 */
    asm volatile ("mov x0, #0x0         \n\t"
                  "msr spsr_el1, x0   \n\t"
                  "msr elr_el1, %0      \n\t"
                  "msr sp_el0, %1       \n\t"
                  "eret                 \n\t" :: "r" (func), "r" (&ustack_pool[current->task_id][USTACK_TOP]):);
}