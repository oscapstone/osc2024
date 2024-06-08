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

    /* Create pgd table first. */
    pgd = create_empty_page_table();
    stack = (unsigned long *) kmalloc(PAGE_SIZE * 4);

    map_pages(pgd, 0x0, size, virt_to_phys((unsigned long)func), 0); // user program is mapped to 0x0
    map_pages(pgd, 0xffffffffb000, PAGE_SIZE * 4, virt_to_phys((unsigned long)stack), 0); // user stack is mapped to 0xffffffffb000

    asm volatile("msr elr_el1, %0" ::"r"(0x0));
    asm volatile("msr sp_el0, %0" ::"r"(USER_STACK_TOP));
    asm volatile("msr spsr_el1, xzr"); // user mode
    asm volatile("dsb ish");
    asm volatile("msr ttbr0_el1, %0" ::"r"(virt_to_phys((unsigned long)pgd)));
    asm volatile("tlbi vmalle1is");
    asm volatile("dsb ish");
    asm volatile("isb");
    asm volatile("eret");
}