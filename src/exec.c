#include "sched.h"
#include "mm.h"
#include "sched.h"

/**
 * Execute a function in user space.
 * func is the physical address of user program, size is the size of user program.
 * Should create kernel task first, then the kernel task calls this function to create user task.
 * Because the address of user program has been allocated. We could just map it to virtual address space.
 */
void do_exec(void (*func)(void), unsigned long size)
{
    unsigned long *pgd;
    unsigned long *stack;

    func();

    printf("[do_exec] func: %x, size: %d\n", func, size);

    /* Create pgd table first. */
    pgd = (unsigned long *) kmalloc(PAGE_SIZE);
    stack = (unsigned long *) kmalloc(PAGE_SIZE * 4);

    map_pages(pgd, 0x0, size, virt_to_phys((unsigned long)func), 0); // user program is mapped to 0x0
    map_pages(pgd, 0xffffffffb000, PAGE_SIZE * 4, virt_to_phys((unsigned long)stack), 0); // user stack is mapped to 0xffffffffb000

    asm volatile("msr elr_el1, %0" ::"r"(0x0));
    asm volatile("msr sp_el0, %0" ::"r"(USER_STACK_TOP));
    asm volatile("mov sp, %0" ::"r"(&kstack_pool[current->task_id][USTACK_TOP])); // not sure why we need to setup sp again
    asm volatile("msr spsr_el1, xzr"); // user mode
    asm volatile("dsb ish");
    asm volatile("msr ttbr0_el1, %0" ::"r"(virt_to_phys((unsigned long)pgd)));
    asm volatile("tlbi vmalle1is");
    asm volatile("dsb ish");
    asm volatile("isb");
    asm volatile("eret");

    /* This user task will have sp == (&ustack_pool[current->task_id][USTACK_TOP]) at el0 */
    // asm volatile("mov x1, #0x0           \n\t"
    //              "msr spsr_el1, x1       \n\t");
    // asm volatile("msr elr_el1, %0" ::"r" (func));
    // asm volatile("msr sp_el0, %0" ::"r" (&ustack_pool[current->task_id][USTACK_TOP]));
    // asm volatile("eret");

    // asm("msr tpidr_el1, %0\n\t"
    //     "msr elr_el1, %1\n\t"
    //     "msr spsr_el1, xzr\n\t" // enable interrupt in EL0. You can do it by setting spsr_el1 to 0 before returning to EL0.
    //     "msr sp_el0, %2\n\t"
    //     "mov sp, %3\n\t"
    //     "dsb ish\n\t" // ensure write has completed
    //     "msr ttbr0_el1, %4\n\t"
    //     "tlbi vmalle1is\n\t" // invalidate all TLB entries
    //     "dsb ish\n\t"        // ensure completion of TLB invalidatation
    //     "isb\n\t"            // clear pipeline"
    //     "eret\n\t" ::"r"(current),
    //     "r"(0x0), "r"(USER_STACK_TOP), "r"(&kstack_pool[current->task_id][USTACK_TOP]), "r"(pgd));
}