
#include "base.h"
#include "mmu.h"
#include "utils/utils.h"
#include "mm/mm.h"
#include "arm/sysregs.h"
#include "utils/printf.h"

void setup_identity_mapping()
{
    // set the default translation config register
    utils_write_sysreg(tcr_el1, TCR_CONFIG_DEFAULT);

    // set the default memory attribute indirection register
    utils_write_sysreg(mair_el1, MAIR_DEFAULT_VALUE);

    // our page table look like this 2 frame
    // first frame PGD frame
    // | PGD
    // second frame PUD frame
    // | PUD kernel memory
    // | PUD device memory (ARM peripherals)
    memset(PD_KERNEL_ENTRY, 0, 0x1000);
    memset(PD_FIRST_PUD_ENTRY, 0, 0x1000);

    U64* table_ptr = (U64*)PD_KERNEL_ENTRY;
    *table_ptr = (PD_KERNEL_PGD_ATTR | PD_FIRST_PUD_ENTRY);  // the address of PUD

    table_ptr = (U64*)PD_FIRST_PUD_ENTRY;
    // map 0
    *table_ptr = (PD_KERNEL_PUD_ATTR | 0);
    table_ptr += 1;
    *table_ptr = (PD_KERNEL_PUD_ATTR | 0x40000000);

    // in kernel space 0x00000000xxxxxxxx = 0xffff0000xxxxxxxx
    utils_write_sysreg(ttbr0_el1, (U64*)PD_KERNEL_ENTRY);
    utils_write_sysreg(ttbr1_el1, (U64*)PD_KERNEL_ENTRY);

    // enabling MMU
    asm volatile("tlbi vmalle1is\n\t");
    asm volatile("dsb ish\n\t");
    asm volatile("isb\n\t");
    U64 sctlr = utils_read_sysreg(sctlr_el1);
    utils_write_sysreg(sctlr_el1, sctlr | SCTLR_MMU_ENABLED);
}

void setup_kernel_space_mapping() {

    /**
     * Use the identity page table setup by setup_identity_mapping
     * ->PGD
     *      ->PUD
     *          |-p0
     *          |-p1
     * three level paging each block 2MB
    */
    pd_t* p0 = MMU_VIRT_TO_PHYS((U64)kzalloc(PD_PAGE_SIZE));

    /**
     * 504 * 2MB = 0x3f000000
    */
    /**
     * 0x00000000 ~ 0x3F000000
    */
    for (int i = 0; i < 504; i++) {
        // i << 21 2M each
        p0[i] = (i << 21) | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK;
    }
    /**
     * 0x3F000000 ~ 0x40000000
    */
    for (int i = 504; i < 512; i++) {
        p0[i] = (i << 21) | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK;
    }
    /**
     * 0x40000000 ~ 0x80000000
    */
    pd_t* p1 = MMU_VIRT_TO_PHYS((U64)kzalloc(PD_PAGE_SIZE));
    for (int i = 0; i < 512; i++) {
        p1[i] = 0x40000000 | (i << 21) | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK;
    }

    // 由于一些编译器优化或者CPU设计的流水线乱序执行，导致最终内存的访问顺序可能和代码中的逻辑顺序不符，所以需要增加内存屏障指令来保证顺序性。
    asm volatile("dsb ish");

    PD_FIRST_PUD_ENTRY_VA[0] = (pd_t)p0 | PD_TABLE;
    PD_FIRST_PUD_ENTRY_VA[1] = (pd_t)p1 | PD_TABLE;
}

//////////////////////////////////
/// for task
//////////////////////////////////
void mmu_map_table_entry(pd_t* pte, U64 v_addr, U64 p_addr, U64 flags) {
    U64 index = v_addr >> PD_PAGE_SHIFT;
    index = index & (PD_PTRS_PER_TABLE - 1);
    // TODO: modify the entry premission 
    U64 entry = p_addr | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | flags | PD_TABLE;
    pd_t* pte_virt = (pd_t*)MMU_PHYS_TO_VIRT((U64)pte);
    pte_virt[index] = entry;
}

/**
 * get the table page to this table
*/
pd_t mmu_map_table(pd_t* table, U64 shift, U64 v_addr, BOOL* gen_new_table) {
    U64 index = v_addr >> shift;
    index = index & (PD_PTRS_PER_TABLE - 1);
    pd_t* table_virt = (pd_t*)MMU_PHYS_TO_VIRT((U64)table);
    if (!table_virt[index]) {        // no entry
        *gen_new_table = TRUE;
        U64 next_level_table = MMU_VIRT_TO_PHYS((U64)kzalloc(PD_PAGE_SIZE));
        U64 entry = next_level_table | PD_ACCESS | PD_TABLE;        // it is a table entry
        table_virt[index] = entry;
        return next_level_table;
    }
    *gen_new_table = FALSE;
    return table_virt[index] & PD_PAGE_MASK;
}

U64 mmu_get_pte(TASK* task, U64 v_addr) {
    U64 pgd;
    if (!task->cpu_regs.pgd) {
        task->cpu_regs.pgd = MMU_VIRT_TO_PHYS((U64)kzalloc(PD_PAGE_SIZE));
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = task->cpu_regs.pgd;
    }
    pgd = task->cpu_regs.pgd;
    BOOL gen_new_table = FALSE;
    U64 pud = mmu_map_table((pd_t*)(pgd + 0), PD_PGD_SHIFT, v_addr, &gen_new_table);
    if (gen_new_table) {
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = pud;
    }
    U64 pmd = mmu_map_table((pd_t*)(pud + 0), PD_PUD_SHIFT, v_addr, &gen_new_table);
    if (gen_new_table) {
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = pmd;
    }
    U64 pte = mmu_map_table((pd_t*)(pmd + 0), PD_PMD_SHIFT, v_addr, &gen_new_table);
    if (gen_new_table) {
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = pte;
    }
    return pte;
}

/**
 * Assign a page to this task virtual address
 * This function create each level table for the target page (save in task kernel pages)
 * the target page are store in vma struct
 * @param task
 *      the task this virtual space use
 * @param v_addr
 *      the virtual address this page will assign to
 * @param page
 *      the page to be insert
 * @param flags
 *      page descriptor flags
 * @return user page
 *      the allocated page physical address
*/
USER_PAGE_INFO* mmu_map_page(TASK* task, U64 v_addr, U64 page, U64 flags) {
    NS_DPRINT("[MMU][DEBUG] mmu map page start, task[%d] v_addr: %x, page: %x.\n", task->pid, v_addr, page);
    U64 pte = mmu_get_pte(task, v_addr);
    mmu_map_table_entry((pd_t*)(pte + 0), v_addr, page, flags);
    U8 user_flags = 0;
    if (flags & MMU_AP_EL0_READ_ONLY) {
        user_flags |= TASK_USER_PAGE_INFO_FLAGS_READ;
    } else if (flags & MMU_AP_EL0_READ_WRITE) {
        user_flags |= TASK_USER_PAGE_INFO_FLAGS_WRITE | TASK_USER_PAGE_INFO_FLAGS_READ;
    }
    if (flags & (1 << 53)) {
        user_flags |= TASK_USER_PAGE_INFO_FLAGS_EXEC;
    }
    USER_PAGE_INFO* user_page = &task->mm.user_pages[task->mm.user_pages_count++];
    user_page->p_addr = page;
    user_page->v_addr = v_addr;
    user_page->flags = user_flags;
    NS_DPRINT("[MMU][DEBUG] mmu map page end.\n");
    return user_page;
}

/***
 * Not record to block descriptors
*/
void mmu_task_init(TASK* task) {

    // initalize the user mode stack for user page table
    U64 stack_ptr = (U64)MMU_VIRT_TO_PHYS((U64)task->user_stack);

    U64 stack_v_addr = MMU_USER_STACK_BASE - TASK_STACK_SIZE;
    for (int i = 0; i < TASK_STACK_PAGE_COUNT; i++) {
        U64 pte = mmu_get_pte(task, stack_v_addr);
        mmu_map_table_entry((pd_t*)(pte + 0), stack_v_addr, stack_ptr + (i * PD_PAGE_SIZE), MMU_AP_EL0_READ_WRITE | MMU_PXN/* 還沒看懂到底要不要家 */);
        stack_v_addr += PD_PAGE_SIZE;
    }
    return;
}

/**
 * @param return
 *      0 = no page found
*/
U64 mmu_get_page(TASK* task, UPTR v_addr) {
    U64 pte = mmu_get_pte(task, v_addr);
    U64* pte_addr = (U64*)pte;
    U64 index = v_addr >> PD_PAGE_SHIFT;
    index = index & (PD_PTRS_PER_TABLE - 1);
    if (!pte_addr[index]) {
        return 0;
    }
    return pte_addr[index] & PD_PAGE_MASK;
}

int mmu_set_entry(TASK* task, U64 v_addr, U64 mmu_flags) {
    U64 pte = mmu_get_pte(task, v_addr);
    U64* pte_addr = (U64*)pte;
    U64 index = v_addr >> PD_PAGE_SHIFT;
    index = index & (PD_PTRS_PER_TABLE - 1);
    if (!pte_addr[index]) {
        return -1;
    }
    U64 page = pte_addr[index] & PD_PAGE_MASK;
    U64 entry = page | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | mmu_flags | PD_BLOCK;
    pte_addr[index] = entry;
    return 0;
}

void mmu_delete_mm(TASK* task) {

    // delete table descriptors
    for (U64 i = 0; i < task->mm.kernel_pages_count; i++) {
        kfree(task->mm.kernel_pages[i]);
    }
    task->mm.kernel_pages_count = 0;
    
    // delete block descriptors
    for (U64 i = 0; i < task->mm.user_pages_count; i++) {
        kfree(task->mm.user_pages[i].p_addr);
    }
    task->mm.user_pages_count = 0;
    task->cpu_regs.pgd = NULL;
    NS_DPRINT("[MMU][DEBUG] task[%d] mm deleted.\n", task->pid);
}

/**
 * Assume new_task has not been initialize
 * kernel task doesn't need the VMA struct
 * 2024/05/16 I have no idea to create a vma to kernel task because it is difficul to know the program size write in kernel (or just I'm so lazy to search the solution)
 *      and that cause the kernel task can't be fork due to the same data space (because doesn't use ELF format
 *  to kernel where is the .text section and where is .data section)
*/
void mmu_fork_mm(TASK* src_task, TASK* new_task) {
    // assmue new_task is pure new task
    for (U64 i = 0; i < src_task->mm.user_pages_count; i++) {
        USER_PAGE_INFO* user_page = &src_task->mm.user_pages[i];
        U64 mmu_flags = 0;

        // TODO: execution premission?

        mmu_flags |= MMU_AP_EL0_READ_ONLY;
        USER_PAGE_INFO* new_task_user_page = mmu_map_page(new_task, user_page->v_addr, user_page->p_addr, mmu_flags);
        // if this page it writable then later the page fault handler will know this and do the copy on write instead of segementation fault.
        new_task_user_page->flags = user_page->flags;
    }
}

