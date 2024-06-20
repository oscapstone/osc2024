#include "bcm2837/rpi_mmu.h"
#include "mmu.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"

void* set_2M_kernel_mmu(void* x0)
{
   // Turn
   //   Two-level Translation (1GB) - in boot.S
   // to
   //   Three-level Translation (2MB) - set PUD point to new table
   unsigned long *pud_table = (unsigned long *) MMU_PUD_ADDR;

    unsigned long *pte_table1 = (unsigned long *) MMU_PTE_ADDR;
    unsigned long *pte_table2 = (unsigned long *)(MMU_PTE_ADDR + 0x1000L);
    for (int i = 0; i < 512; i++)
    {
        unsigned long addr = 0x200000L * i;
        if ( addr >= PERIPHERAL_END )
        {
            pte_table1[i] = ( 0x00000000 + addr ) + BOOT_PTE_ATTR_nGnRnE;
            continue;
        }
        pte_table1[i] = (0x00000000 + addr ) | BOOT_PTE_ATTR_NOCACHE; //   0 * 2MB // No definition for 3-level attribute, use nocache.
        pte_table2[i] = (0x40000000 + addr ) | BOOT_PTE_ATTR_NOCACHE; // 512 * 2MB
    }

    // set PUD
    pud_table[0] = (unsigned long) pte_table1 | BOOT_PUD_ATTR;
    pud_table[1] = (unsigned long) pte_table2 | BOOT_PUD_ATTR;

    return x0;
}

void map_one_page(size_t *virt_pgd_p, size_t va, size_t pa, size_t flag)
{
    size_t *table_p = virt_pgd_p;
    for (int level = 0; level < 4; level++)
    {
        unsigned int idx = (va >> (39 - level * 9)) & 0x1ff; // p.14, 9-bit only

        if (level == 3)
        {
            table_p[idx] = pa;
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | flag; // el0 only
            return;
        }

        if(!table_p[idx])
        {
            size_t* newtable_p =kmalloc(0x1000);             // create a table
            memset(newtable_p, 0, 0x1000);
            table_p[idx] = VIRT_TO_PHYS((size_t)newtable_p); // point to that table
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }

        table_p = (size_t*)PHYS_TO_VIRT((size_t)(table_p[idx] & ENTRY_ADDR_MASK)); // PAGE_SIZE
    }
}

// 增加virtual meomry address
void mmu_add_vma(struct thread *t, size_t va, size_t size, size_t pa, size_t rwx, int is_alloced)
{
    // align page size
    // 4KB
    size = size % 0x1000 ? size + (0x1000 - size % 0x1000) : size;
    
    // initial新的VMA structure
    vm_area_struct_t* new_area = kmalloc(sizeof(vm_area_struct_t));
    new_area->rwx = rwx;
    new_area->area_size = size;
    new_area->virt_addr = va;
    new_area->phys_addr = pa;
    new_area->is_alloced = is_alloced;
    
    // 將新的VMA加到list
    list_add_tail((list_head_t *)new_area, &t->vma_list);
}

// del. VMA: parse thread的VMA list，free所有已分配的memory並 del. VMA結構
void mmu_del_vma(struct thread *t)
{
    list_head_t *pos = t->vma_list.next;
    vm_area_struct_t *vma;
    
    while(pos != &t->vma_list){
        vma = (vm_area_struct_t *)pos;
        // 如果VMA是已分配的，free memory
        if (vma->is_alloced)
            kfree((void*)PHYS_TO_VIRT(vma->phys_addr));
            
        // free VMA structure
        list_head_t* next_pos = pos->next;
        kfree(pos);
        pos = next_pos;
    }
}

void mmu_map_pages(size_t *virt_pgd_p, size_t va, size_t size, size_t pa, size_t flag)
{
    pa = pa - (pa % 0x1000); // align
    for (size_t s = 0; s < size; s+=0x1000)
    {
        map_one_page(virt_pgd_p, va + s, pa + s, flag);
    }
}

// recursive free page table的meomry，確保所有page table entries被clear並free meomry
void mmu_free_page_tables(size_t *page_table, int level)
{
    size_t *table_virt = (size_t*)PHYS_TO_VIRT((char*)page_table);
    for (int i = 0; i < 512; i++)
    {
        if (table_virt[i] != 0)
        {
            size_t *next_table = (size_t*)(table_virt[i] & ENTRY_ADDR_MASK);
            
            // 如果entry是page table，free next level page table
            if (table_virt[i] & PD_TABLE)
            {
                if(level!=2) mmu_free_page_tables(next_table, level + 1);
                
                // clear entry並free meomry
                table_virt[i] = 0L;
                kfree(PHYS_TO_VIRT((char *)next_table));
            }
        }
    }
}

// 用於處理memory access fail的情況，translation fault
void mmu_memfail_abort_handle(esr_el1_t* esr_el1)
{
    // far_el1: Fault address register
    // 從 FAR_EL1 reg中得到fault的VMA far_el1。
    unsigned long long far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t": "=r"(far_el1));

    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = 0;
    // 遍歷current thread的VMA list，包含 far_el1 address
    list_for_each(pos, &curr_thread->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        // 如果找到了對應的region，記錄該region的 the_area_ptr。
        if (vma->virt_addr <= far_el1 && vma->virt_addr + vma->area_size >= far_el1)
        {
            the_area_ptr = vma;
            break;
        }
    }
    
    // 如果沒有找到對應的region，region不在process的address space
    //則發送segmentation fault並terminates該process
    // area is not part of process's address space
    if (!the_area_ptr)
    {
        uart_sendline("[Segmentation fault]: Kill Process\r\n");
        thread_exit();
        return;
    }

    // 如果是translation fault(TF_LEVEL0 到 TF_LEVEL3), 則只為fault address map一個page frame
    if ((esr_el1->iss & 0x3f) == TF_LEVEL0 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL1 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL2 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL3)
    {
        uart_sendline("[Translation fault]: 0x%x\r\n",far_el1); 
                                           // Holds the faulting Virtual Address for all synchronous Instruction or Data Abort, PC alignment fault and Watchpoint exceptions that are taken to EL1.
        
        // 計算fault address相對於region起始地址的偏移量 addr_offset，並將其對齊到page size
        size_t addr_offset = (far_el1 - the_area_ptr->virt_addr);
        addr_offset = (addr_offset % 0x1000) == 0 ? addr_offset : addr_offset - (addr_offset % 0x1000);

        size_t flag = 0;
        // 如果區域不可執行，set PD_UNX
        if(!(the_area_ptr->rwx & (0b1 << 2))) flag |= PD_UNX;        // 4: executable
        // 如果區域only read，set PD_RDONLY
        if(!(the_area_ptr->rwx & (0b1 << 1))) flag |= PD_RDONLY;     // 2: writable
        // 如果區域可read write，set PD_UK_ACCESS
        if(  the_area_ptr->rwx & (0b1 << 0) ) flag |= PD_UK_ACCESS;  // 1: readable / accessible
        // 將VMA map to PMA，設置相應的page attributes
        map_one_page(PHYS_TO_VIRT(curr_thread->context.pgd), the_area_ptr->virt_addr + addr_offset, the_area_ptr->phys_addr + addr_offset, flag);
    }
    else{
        // 如果translation fault, 則是segmentation fault並exit process
        // For other Fault (permisson ...etc)
        uart_sendline("[Segmentation fault]: Kill Process\r\n");
        thread_exit();
    }

}
