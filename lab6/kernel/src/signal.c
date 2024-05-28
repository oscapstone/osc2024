#include "uart.h"
#include "schedule.h"
#include "allocator.h"
#include "exception.h"
#include "mm.h"
#include "signal.h"

void default_SIGKILL_handler()
{
    task_exit();
}

void do_signal(struct ucontext *sigframe, void (*signal_handler)(void))
{
    task_struct *cur = get_current_task();

    disable_interrupt();
    cur->signal_stack = (void *)((char *)kmalloc(4096 * 5) + 4096 * 4); // malloc a stack's space to deal with signal

    page_reclaim(cur->mm_struct->pgd);                                                                 // reclaim all page entry
    cur->mm_struct->pgd = (unsigned long long *)((unsigned long long)create_page_table() & ~VA_START); // allocate a new pgd page table

    mmap_pop(&(cur->mm_struct->mmap), STACK); // pop the user stack form vma
    // map signal stack
    mappages(cur->mm_struct, STACK, 0xffffffffb000, (unsigned long long)(cur->signal_stack) - 4096 * 4 - VA_START, 4096 * 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS);
    // map sigreturn system call
    mappages(cur->mm_struct, SYSCALL, (unsigned long long)sigreturn - VA_START, (unsigned long long)sigreturn - VA_START, 4096, PROT_READ | PROT_EXEC, MAP_ANONYMOUS);

    switch_mm_irqs_off(cur->mm_struct->pgd); // flush tlb and pipeline because page table is change

    unsigned long long *sp_ptr = cur->signal_stack;

    for (int i = 0; i < 34; i++)
        *(sp_ptr - i) = *((unsigned long long *)sigframe + i); // push sigframe to signal stack
    sp_ptr -= 34;

    asm volatile(
        "msr sp_el0, %0\n"
        "msr elr_el1, %1\n"
        "mov x10, 0\n"
        "msr spsr_el1, x10\n"
        "mov lr, %2\n" // set lr to sigreturn to restore context
        "mov sp, %3\n"
        "eret\n"
        :
        : "r"(0xfffffffff000 - 34 * 8), "r"(signal_handler), "r"((unsigned long long)sigreturn - VA_START), "r"(cur->kstack));
}
