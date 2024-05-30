
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
 * @param table
 *      in physical address
*/
pd_t mmu_map_table(pd_t* table, U64 shift, U64 v_addr, BOOL* gen_new_table) {
    U64 index = v_addr >> shift;
    index = index & (PD_PTRS_PER_TABLE - 1);
    pd_t* table_virt = (pd_t*)MMU_PHYS_TO_VIRT((U64)table);
    //////////////
    /// WIRED bug it didn't translate to virtual address!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    printf("table phy addr: 0x%x, virt addr: 0x%x%x\n", table, (U64)table_virt >> 32, table_virt);
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

U64 mmu_get_pud(TASK* task, U64 v_addr) {
    U64 pgd;
    if (!task->cpu_regs.pgd) {
        task->cpu_regs.pgd = MMU_VIRT_TO_PHYS((U64)kzalloc(PD_PAGE_SIZE));
        NS_DPRINT("[MMU][TRACE] new PGD table for task. pid = %d, addr: 0x%x\n", task->pid, task->cpu_regs.pgd);
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = task->cpu_regs.pgd;
    }
    pgd = task->cpu_regs.pgd;
    NS_DPRINT("[MMU][TRACE] PGD table. pid = %d, addr: 0x%x%x\n", task->pid, task->cpu_regs.pgd >> 32, task->cpu_regs.pgd);
    BOOL gen_new_table = FALSE;
    U64 pud = mmu_map_table(pgd, PD_PGD_SHIFT, v_addr, &gen_new_table);
    if (gen_new_table) {
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = pud;
    }
    return pud;
}

U64 mmu_get_pmd(TASK* task, U64 v_addr) {
    BOOL gen_new_table = FALSE;
    U64 pud = mmu_get_pud(task, v_addr);
    if (gen_new_table) {
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = pud;
    }
    U64 pmd = mmu_map_table((pd_t*)(pud + 0), PD_PUD_SHIFT, v_addr, &gen_new_table);
    if (gen_new_table) {
        task->mm.kernel_pages[task->mm.kernel_pages_count++] = pmd;
    }
    return pmd;
}

U64 mmu_get_pte(TASK* task, U64 v_addr) {
    U64 pmd = mmu_get_pmd(task, v_addr);
    BOOL gen_new_table = FALSE;
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
 *      the page to be insert in physical address
 * @param flags
 *      page descriptor flags
 * @return user page
 *      the allocated page physical address
*/
USER_PAGE_INFO* mmu_map_page(TASK* task, U64 v_addr, U64 page, U64 flags) {
    NS_DPRINT("[MMU][DEBUG] mmu map page start, task[%d] v_addr: %x, page: %x.\n", task->pid, v_addr, page);
    U64 pte = mmu_get_pte(task, v_addr);
    mmu_map_table_entry((pd_t*)(pte), v_addr, page, flags);
    U8 user_flags = 0;
    
    if (flags & MMU_AP_EL0_UK_ACCESS) {
        user_flags |= TASK_USER_PAGE_INFO_FLAGS_READ;
        if (!(flags & MMU_AP_EL0_READ_ONLY)) {
            user_flags |= TASK_USER_PAGE_INFO_FLAGS_WRITE;   
        }
    }
    if (!(flags & MMU_UNX)) {
        user_flags |= TASK_USER_PAGE_INFO_FLAGS_EXEC;
    }
    USER_PAGE_INFO* user_page = NULL;
    for (U64 i = 0; i < task->mm.user_pages_count; i++) {
        USER_PAGE_INFO* cur_page = &task->mm.user_pages[i];
        if (cur_page->v_addr == v_addr) {
            user_page = cur_page;
            break;
        }
    }
    if (user_page == NULL) {
        NS_DPRINT("[MMU][DEBUG] New user page. v_addr: 0x%x, p_addr: 0x%x\n", v_addr, page);
        user_page = &task->mm.user_pages[task->mm.user_pages_count++];
    }

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
        mmu_map_table_entry((pd_t*)(pte + 0), stack_v_addr, stack_ptr + (i * PD_PAGE_SIZE), MMU_AP_EL0_UK_ACCESS | MMU_UNX /* stack為不可執行 */ | MMU_PXN/* 還沒看懂到底要不要家: 要加因為user stack不可在EL1執行*/);
        stack_v_addr += PD_PAGE_SIZE;
    }

    // 這個玩具OS可以讓應用程式隨意存取peripheral的I/O喔 這麼蝦
    U64 peripheral_ptr = 0x3c000000;
    for (int i = 0; i < 24; i++) {
        U64 current_peripheral_addr = peripheral_ptr + (i << 21) /* 2M for here */;
        U64 index = current_peripheral_addr >> PD_PMD_SHIFT;
        index = index & (PD_PTRS_PER_TABLE - 1);
        //NS_DPRINT("!!!!!!!!!!!!!!! index: %d\n", index);
        U64 pmd = mmu_get_pmd(task, current_peripheral_addr);
        pd_t* pmd_virt = (pd_t*)MMU_PHYS_TO_VIRT((U64)pmd);
        pmd_virt[index] = current_peripheral_addr | MMU_AP_EL0_UK_ACCESS | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK | MMU_UNX | MMU_PXN/* can not execute */;
    }

    return;
}

/**
 * @param return
 *      get physical address of the page
 *      0 = no page found
*/
U64 mmu_get_page(TASK* task, UPTR v_addr) {
    U64 pte = mmu_get_pte(task, v_addr);
    pd_t* pte_virt = (pd_t*)MMU_PHYS_TO_VIRT((U64)pte);
    U64 index = v_addr >> PD_PAGE_SHIFT;
    index = index & (PD_PTRS_PER_TABLE - 1);
    if (!pte_virt[index]) {
        return 0;
    }
    return pte_virt[index] & PD_PAGE_MASK;
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
        if (task->mm.user_pages[i].p_addr)      // because it probably not loaded yet.
            mem_dereference(task->mm.user_pages[i].p_addr);
    }
    task->mm.user_pages_count = 0;

    // delete mmap
    // VMA_STRUCT* mmap = task->mm.mmap;
    // while (mmap) {
    //     VMA_STRUCT* next = mmap->next;
    //     kfree(mmap);
    //     mmap = next;
    // }

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

        mmu_flags |= MMU_PXN;                       // cannot be execute in kernel mode
        if (user_page->flags & TASK_USER_PAGE_INFO_FLAGS_READ) {
            mmu_flags |= MMU_AP_EL0_UK_ACCESS;      // can be access in user mode
        }
        mmu_flags |= MMU_AP_EL0_READ_ONLY;          // only readable for child process
        if (!(user_page->flags & TASK_USER_PAGE_INFO_FLAGS_EXEC)) {
            mmu_flags |= MMU_UNX;                   // cannot be execute in user mode
        }

        USER_PAGE_INFO* new_task_user_page = mmu_map_page(new_task, user_page->v_addr, user_page->p_addr, mmu_flags);
        mem_reference(user_page->p_addr);
        // if this page it writable then later the page fault handler will know this and do the copy on write instead of segementation fault.
        new_task_user_page->flags = user_page->flags;
    }
}

void mmu_memfail_handler(U64 esr) {
    TASK* task = task_get_current_el1();        // get the current task

    U64 far_el1 = utils_read_sysreg(FAR_EL1);
    USER_PAGE_INFO* current_page_info = NULL;
    for (U32 i = 0; i < TASK_MAX_USER_PAGES; i++) {
        USER_PAGE_INFO* page_info = &task->mm.user_pages[i];
        if (page_info->v_addr <= far_el1 && page_info->v_addr + PD_PAGE_SIZE >= far_el1) {
            current_page_info = page_info;
            break;
        }
    }

    U64 iss = ESR_ELx_ISS(esr);
    NS_DPRINT("iss: 0x%x\n", iss);

    if (!current_page_info) {
        NS_DPRINT("[MMU][ERROR] [Segmentation fault: Kill process]\n");
        NS_DPRINT("addr: %x\n", far_el1);
        task_exit(-1);
        // will not go anywhere
    }

    // not assign page for it, it is damand paging (anonymous page allocation)
    if (current_page_info->p_addr == 0) {
        NS_DPRINT("[MMU][WARN] [Translation fault]: 0x%x\n", far_el1);
        NS_DPRINT("[MMU][TRACE] page not present yet.\n");
        U64 page_flags = 0;
        page_flags |= MMU_PXN;
        if (current_page_info->flags & TASK_USER_PAGE_INFO_FLAGS_READ) {
            page_flags |= MMU_AP_EL0_UK_ACCESS;
        }
        if (!(current_page_info->flags & TASK_USER_PAGE_INFO_FLAGS_WRITE)) {
            page_flags |= MMU_AP_EL0_READ_ONLY;
        }
        if (!(current_page_info->flags & TASK_USER_PAGE_INFO_FLAGS_EXEC)) {
            page_flags |= MMU_UNX;
        }
        U64 new_page = kzalloc(PD_PAGE_SIZE);
        mmu_map_page(task, current_page_info->v_addr, MMU_VIRT_TO_PHYS(new_page), page_flags);
        return;
    }

    // is copy on write
    if (ISS_IS_WRITE(iss) && (current_page_info->flags & TASK_USER_PAGE_INFO_FLAGS_WRITE)) {
        NS_DPRINT("[MMU][TRACE] Writing operation permission fault and can be write, doing copy on write.\n");
        U64 page_flags = 0;       // flag for page descriptor entry
        page_flags |= MMU_PXN;      // cannot be executed in kernel mode
        page_flags |= MMU_AP_EL0_UK_ACCESS; // can write not can read is wired
        if (!(current_page_info->flags & TASK_USER_PAGE_INFO_FLAGS_EXEC)) page_flags |= MMU_UNX;
        NS_DPRINT("[MMU][TRACE] origin page addr: 0x%x\n", current_page_info->p_addr);
        U64 new_page = kmalloc(PD_PAGE_SIZE);
        memcpy((void*)MMU_PHYS_TO_VIRT(current_page_info->p_addr), (void*)new_page, PD_PAGE_SIZE);
        mem_dereference(current_page_info->p_addr);     // detach the old page (parent page)

        // replace the page to modify the new page
        NS_DPRINT("[MMU][TRACE] new page addr: 0x%x%x\n", new_page >> 32, new_page);
        mmu_map_page(task, current_page_info->v_addr, MMU_VIRT_TO_PHYS(new_page), page_flags);
        return;
    }

    if ((iss & 0x3f) == ISS_TF_LEVEL0 ||
        (iss & 0x3f) == ISS_TF_LEVEL1 ||
        (iss & 0x3f) == ISS_TF_LEVEL2 ||
        (iss & 0x3f) == ISS_TF_LEVEL3)
    {
        // Will not go here probably?
    } else {
        printf("[MMU][ERROR] [Segmentation fault]: other fault.\n");
        printf("iss: 0x%x\n", iss);
        printf("addr: %x\n", far_el1);
        task_exit(-1);
    }



}

void* mmu_va2pa(UPTR v_addr) {

    UPTR page_p_addr = mmu_get_page(task_get_current_el1(), v_addr);

    if (!page_p_addr) {
        NS_DPRINT("[MMU][ERROR] Failed to get the page from virtual address: %x\n", v_addr);
        return 0;
    }

    U64 offset = v_addr & 0xfff;
    
    return (offset | page_p_addr);
}

// int mmu_vma_alloc(TASK *task, UPTR v_start, U32 page_count, U32 flags) {
//     if (v_start & 0xfff) {
//         v_start = v_start & ~(0xfff);
//     }

//     VMA_STRUCT* mmap = kzalloc(sizeof(VMA_STRUCT));
//     mmap->v_start = v_start;
//     mmap->v_end = v_start + page_count * PD_PAGE_SIZE;
//     mmap->flags = flags;
//     mmap->next = NULL;
//     NS_DPRINT("[MMU][TRACE] start mapping vma. v_start: 0x%x", v_start);

//     if (!task->mm.mmap) {
//         task->mm.mmap = mmap;
//         return 0;       // success
//     }

//     if (task->mm.mmap->v_start > mmap->v_end) {
//         mmap->next = task->mm.mmap;
//         task->mm.mmap = mmap;
//         return 0;       // success
//     }

//     VMA_STRUCT* previous_Mmap = task->mm.mmap;
//     VMA_STRUCT* current_Mmap = task->mm.mmap->next;

//     // check if intersect at the first mmap
//     if (previous_Mmap->v_start < mmap->v_start && previous_Mmap->v_end > mmap->v_start ||
//         previous_Mmap->v_start < mmap->v_end && previous_Mmap->v_end > mmap->v_end
//         ) {
//         kfree(mmap);
//         return -1;
//     }

//     while (current_Mmap) {

//         if (previous_Mmap->v_end < mmap->v_start && current_Mmap->v_start > mmap->v_end) {
//             previous_Mmap->next = mmap;
//             mmap->next = current_Mmap;
//             return 0;
//         }

//         // intersect
//         if (previous_Mmap->v_start < mmap->v_start && previous_Mmap->v_end > mmap->v_start ||
//             previous_Mmap->v_start < mmap->v_end && previous_Mmap->v_end > mmap->v_end
//             ) {
//             kfree(mmap);
//             return -1;
//         }

//         previous_Mmap = current_Mmap;
//         current_Mmap = current_Mmap->next;
//     }

//     previous_Mmap->next = mmap;
//     return 0;
// }