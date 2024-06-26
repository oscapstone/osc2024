#include "mm.h"
#include "arm/mmu.h"
#include "fork.h"
#include "list.h"
#include "memory.h"
#include "page_alloc.h"
#include "sched.h"
#include "slab.h"

struct vm_area_struct* find_vm_area(struct task_struct* task,
                                    enum vm_type vm_type)
{
    struct vm_area_struct* vm_area;
    list_for_each_entry (vm_area, &task->mm.mmap_list, list) {
        if (vm_area->vm_type == vm_type)
            return vm_area;
    }
    return NULL;
}

void add_vm_area(struct task_struct* task,
                 enum vm_type vm_type,
                 unsigned long va_start,
                 unsigned long pa_start,
                 unsigned long area_sz)
{
    struct vm_area_struct* vm_area = kmalloc(sizeof(struct vm_area_struct), 0);
    vm_area->vm_type = vm_type;
    vm_area->va_start = va_start;
    vm_area->pa_start = pa_start;
    vm_area->area_sz = area_sz;
    list_add(&vm_area->list, &task->mm.mmap_list);
}

unsigned long allocate_kernel_pages(size_t size, gfp_t flags)
{
    unsigned long page = (unsigned long)kmalloc(size, flags);
    if (!page)
        return 0;
    return page;
}

unsigned long allocate_user_pages(struct task_struct* task,
                                  enum vm_type vm_type,
                                  unsigned long va,
                                  size_t size,
                                  gfp_t flags)
{
    unsigned long page = (unsigned long)kmalloc(size, flags);
    if (!page)
        return 0;
    map_pages(task, vm_type, va, page, size);
    return page;
}

void map_table_entry(unsigned long* pte, unsigned long va, unsigned long pa)
{
    unsigned long index = va >> PAGE_SHIFT;
    index &= (PTRS_PER_TABLE - 1);
    unsigned long entry = pa | MMU_PTE_FLAGS;
    pte[index] = entry;
}


unsigned long map_table(unsigned long* table,
                        unsigned long shift,
                        unsigned long va,
                        int* new_table)
{
    unsigned long index = va >> shift;
    index &= (PTRS_PER_TABLE - 1);
    if (!table[index]) {
        *new_table = 1;
        unsigned long next_level_table =
            (unsigned long)kzmalloc(PAGE_SIZE, 0) - VA_START;
        unsigned long entry = next_level_table | MM_TYPE_PAGE_TABLE;
        table[index] = entry;
        return next_level_table;
    }

    *new_table = 0;
    return table[index] & PAGE_MASK;
}

void map_page(struct task_struct* task, unsigned long va, unsigned long page)
{
    if (task->mm.pgd == pg_dir) {
        task->mm.pgd = (unsigned long)kzmalloc(PAGE_SIZE, 0) - VA_START;
        add_vm_area(task, TABLE, task->mm.pgd + VA_START, task->mm.pgd,
                    PAGE_SIZE);
    }

    unsigned long pgd = task->mm.pgd;

    int new_table;
    unsigned long pud =
        map_table((unsigned long*)(pgd + VA_START), PGD_SHIFT, va, &new_table);
    if (new_table)
        add_vm_area(task, TABLE, pud + VA_START, pud, PAGE_SIZE);


    unsigned long pmd =
        map_table((unsigned long*)(pud + VA_START), PUD_SHIFT, va, &new_table);
    if (new_table)
        add_vm_area(task, TABLE, pmd + VA_START, pmd, PAGE_SIZE);

    unsigned long pte =
        map_table((unsigned long*)(pmd + VA_START), PMD_SHIFT, va, &new_table);
    if (new_table)
        add_vm_area(task, TABLE, pte + VA_START, pte, PAGE_SIZE);

    map_table_entry((unsigned long*)(pte + VA_START), va, page - VA_START);
}

void map_pages(struct task_struct* task,
               enum vm_type vm_type,
               unsigned long va,
               unsigned long page,
               size_t size)
{
    // size_t nr_pages = (size >> PAGE_SHIFT) + !!(size & (PAGE_SIZE - 1));
    size_t nr_pages = 1 << get_order(size);
    for (int i = 0; i < nr_pages; i++) {
        size_t offset = i << PAGE_SHIFT;
        map_page(task, va + offset, page + offset);
    }
    add_vm_area(task, vm_type, va, page - VA_START, nr_pages << PAGE_SHIFT);
}

int copy_virt_memory(struct task_struct* dst)
{
    struct task_struct* src = current_task;

    struct vm_area_struct* vm_area;
    list_for_each_entry (vm_area, &src->mm.mmap_list, list) {
        if (vm_area->vm_type == IO || vm_area->vm_type == TABLE)
            continue;
        unsigned long kernel_va = allocate_user_pages(
            dst, vm_area->vm_type, vm_area->va_start, vm_area->area_sz, 0);
        if (!kernel_va)
            return -1;
        memcpy((void*)kernel_va, (const void*)vm_area->va_start,
               vm_area->area_sz);
    }

    map_pages(dst, IO, IO_PM_START_ADDR, IO_PM_START_ADDR,
              IO_PM_END_ADDR - IO_PM_START_ADDR);
    add_vm_area(dst, IO, IO_PM_START_ADDR, IO_PM_START_ADDR,
                IO_PM_END_ADDR - IO_PM_START_ADDR);

    return 0;
}

static int ind = 1;

int do_mem_abort(unsigned long addr, unsigned long esr)
{
    unsigned long dfs = (esr & 0b111111);
    if ((dfs & 0b111100) == 0b100) {
        unsigned long page = (unsigned long)kmalloc(PAGE_SIZE, 0);
        if (!page)
            return -1;
        map_page(current_task, addr & PAGE_MASK, page);
        ind++;
        if (ind > 2)
            return -1;
        return 0;
    }
    return -1;
}


unsigned long* find_page_entry(struct task_struct* task, unsigned long va)
{
    if (task->mm.pgd == pg_dir)
        return 0;

    unsigned long* pgd = (unsigned long*)task->mm.pgd;
    unsigned long pgd_idx = (va >> PGD_SHIFT) & (PTRS_PER_TABLE - 1);

    unsigned long* pud = (unsigned long*)pgd[pgd_idx];
    if (!pud)
        return 0;

    unsigned long pud_idx = (va >> PUD_SHIFT) & (PTRS_PER_TABLE - 1);
    unsigned long* pmd = (unsigned long*)pud[pud_idx];
    if (!pmd)
        return 0;

    unsigned long pmd_idx = (va >> PMD_SHIFT) & (PTRS_PER_TABLE - 1);
    unsigned long* pte = (unsigned long*)pmd[pmd_idx];
    if (!pte)
        return 0;

    unsigned long pte_idx = (va >> PAGE_SHIFT) & (PTRS_PER_TABLE - 1);
    return pte + pte_idx;
}

void invalidate_page(struct task_struct* task, unsigned long va)
{
    unsigned long* entry = find_page_entry(task, va);
    *entry = 0;
}

void invalidate_pages(struct task_struct* task, unsigned long va, size_t size)
{
    size_t nr_pages = 1 << get_order(size);
    for (size_t i = 0; i < nr_pages; i++) {
        size_t offset = i << PAGE_SHIFT;
        invalidate_page(task, va + offset);
    }
}
