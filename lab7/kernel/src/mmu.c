#include "bcm2837/rpi_mmu.h"
#include "mmu.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "colourful.h"

extern int uart_recv_echo_flag; // from uart1.c

// #define DEBUG
#ifdef DEBUG
    #define mmu_sendline(fmt, args ...) uart_sendline(fmt, ##args)
    // #define mmu_sendline(fmt, args ...) uart_puts(fmt, ##args)
#else
    #define mmu_sendline(fmt, args ...) (void)0
#endif
// https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-
char *IFSC_table[] = {
    "Address size fault, level 0 of translation or translation table base register.",                           // 0b000000
    "Address size fault, level 1",                                                                              // 0b000001
    "Address size fault, level 2",                                                                              // 0b000010
    "Address size fault, level 3",                                                                              // 0b000011
    "Translation fault, level 0",                                                                               // 0b000100
    "Translation fault, level 1",                                                                               // 0b000101
    "Translation fault, level 2",                                                                               // 0b000110
    "Translation fault, level 3",                                                                               // 0b000111
    "",                                                                                                         // 0b001000                                          
    "Access flag fault, level 1",                                                                               // 0b001001
    "Access flag fault, level 2",                                                                               // 0b001010
    "Access flag fault, level 3",                                                                               // 0b001011
    "Permission fault, level 0",                                                                                // 0b001100
    "Permission fault, level 1",                                                                                // 0b001101
    "Permission fault, level 2",                                                                                // 0b001110
    "Permission fault, level 3"                                                                                // 0b001111
    // "Synchronous External abort, not on translation table walk or hardware update of translation table.",       // 0b010000
    // "Synchronous External abort on translation table walk or hardware update of translation table, level -2.",  // 0b010001
    // ""                                                                                                          // 0b010010
    // "Synchronous External abort on translation table walk or hardware update of translation table, level -1.",  // 0b010011
    // "Synchronous External abort on translation table walk or hardware update of translation table, level 0.",   // 0b010100
    // "Synchronous External abort on translation table walk or hardware update of translation table, level 1.",   // 0b010101
    // "Synchronous External abort on translation table walk or hardware update of translation table, level 2.",   // 0b010110
    // "Synchronous External abort on translation table walk or hardware update of translation table, level 3.",   // 0b010111
    // "Synchronous parity or ECC error on memory access, not on translation table walk."                          // 0b011000
};

char *VMA_prot_table[] = {              //   xwr
    "No access",                        // 0b000
    "readable",                         // 0b001
    "writable",                         // 0b010
    "writable/readable",                // 0b011
    "executable",                       // 0b100
    "executable/readable",              // 0b101
    "executable/writable",              // 0b110
    "executable/writable/readable"      // 0b111
};


/*
    the function is used to set the 2M kernel mmu
*/
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
        unsigned long addr = 0x200000L * i; // 0x200000L = 2MB
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


/*
    the function is used to map one page
    @param virt_pgd_p: the pointer of the page table
    @param va: the virtual address
    @param pa: the physical address
    @param flag: the flag of the page
*/
void map_one_page(size_t *virt_pgd_p, size_t va, size_t pa, size_t flag)
{
    // lock();
    size_t *table_p = virt_pgd_p;
    for (int level = 0; level < 4; level++)
    {
        unsigned int idx = (va >> (39 - level * 9)) & 0x1ff; // p.14, 9-bit only
        if (level == 3)
        {
            // align 11 bits
            pa = pa - (pa % 0x1000);
            table_p[idx] = pa;
            table_p[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | flag; // el0 only
            // table_p[idx] |= PD_ACCESS | PD_BLOCK | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | flag; // el0 only
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
    // unlock();
}


/*
    the function is used to add a vma to the list
    @param t: the thread
    @param va: the virtual address
    @param size: the size of the vma
    @param pa: the physical address
    @param rwx: the permission of the vma
    @param name: the name of the vma
    @param is_alloced: the flag of the vma
*/
void mmu_add_vma(struct thread *t, size_t va, size_t size, size_t pa, size_t rwx, char *name, int is_alloced)
{
    size = size % 0x1000 ? size + (0x1000 - size % 0x1000) : size;
    if (size == 0)
        return;

    if (strlen(name) > 31)
    {
        uart_sendline(RED "The name of vma is too long\r\n" RESET);
        return;
    }


    // add 1 to the attribute counter from frame_array
    frame_t *frame_array = get_frame_array();
    int frame_idx = (((unsigned long long)(pa) - BUDDY_MEMORY_BASE) >> 12);
    for (int i = 0; i < size / 0x1000; i++)
    {
        frame_array[frame_idx + i].counter++;
    }


    vm_area_struct_t* new_area = kmalloc(sizeof(vm_area_struct_t));
    new_area->rwx           = rwx;
    new_area->area_size     = size;
    new_area->virt_addr     = va;
    new_area->phys_addr     = pa;
    new_area->is_alloced    = is_alloced;
    strcpy(new_area->name, name);
    int index = strlen(new_area->name);
    while(index < 31)
    {
        new_area->name[index++] = ' ';
    }
    new_area->name[31] = '\0';


    // I want to add the new vma to the list by the order of the virt_addr
    list_head_t *curr;
    vm_area_struct_t *vma;
    list_for_each(curr, &t->vma_list)
    {
        vma = (vm_area_struct_t *)curr;
        if (vma->virt_addr > va)
        {
            list_add(&new_area->listhead, curr->prev);
            return;
        }
    }
    list_add_tail((list_head_t *)new_area, &t->vma_list);
}


/*
    the function is used to delete the vma from the list
    @param t: the thread
*/
void mmu_del_vma(struct thread *t)
{
    list_head_t *pos = t->vma_list.next;
    vm_area_struct_t *vma;
    while(pos != &t->vma_list){
        vma = (vm_area_struct_t *)pos;
        if (vma->is_alloced)
            kfree((void*)PHYS_TO_VIRT(vma->phys_addr));
        list_head_t* next_pos = pos->next;
        kfree(pos);
        pos = next_pos;
    }
}

/*
    the function is used to map pages
    @param virt_pgd_p: the pointer of the page table
    @param va: the virtual address
    @param size: the size of the pages
    @param pa: the physical address
    @param flag: the flag of the pages
*/
void mmu_map_pages(size_t *virt_pgd_p, size_t va, size_t size, size_t pa, size_t flag)
{
    pa = pa - (pa % 0x1000); // align
    for (size_t s = 0; s < size; s+=0x1000)
    {
        map_one_page(virt_pgd_p, va + s, pa + s, flag);
    }
}

/*
    the function is used to free page tables
    @param page_table: the pointer of the page table
    @param level: the level of the page table

*/
void mmu_free_page_tables(size_t *page_table, int level)
{
    size_t *table_virt = (size_t*)PHYS_TO_VIRT((char*)page_table);
    for (int i = 0; i < 512; i++)
    {
        if (table_virt[i] != 0)
        {
            size_t *next_table = (size_t*)(table_virt[i] & ENTRY_ADDR_MASK);
            if (table_virt[i] & PD_TABLE)
            {
                if(level!=2)mmu_free_page_tables(next_table, level + 1);
                table_virt[i] = 0L;
                kfree((void*)PHYS_TO_VIRT((char *)next_table));
            }
        }
    }
}

// Impliment copy-and-write `

/*
    To set the page tables to read only for parent and child tables
    @param parent_table: the pointer of the parent table
    @param child_table: the pointer of the child table
    @param level: the level of the page table
*/
void mmu_reset_page_tables_read_only(size_t *parent_table, size_t *child_table, int level)
{
    if (level > 3)
        return;

    size_t *parent_table_virt = (size_t*)PHYS_TO_VIRT((char*)parent_table);
    size_t *child_table_virt = (size_t*)PHYS_TO_VIRT((char*)child_table);
    // int counter_ = 0;
    lock();
    for (int i = 0; i < 512; i++)
    {
        if (parent_table_virt[i] == 0)
            continue;

        if (! (parent_table_virt[i] & PD_TABLE))
        {
            uart_sendline("mmu_reset_page_tables_read_only error\r\n");
            return; 
        }
        if (level == 3){
            child_table_virt[i] = parent_table_virt[i]; 
            child_table_virt[i] |= PD_RDONLY;
            // if (counter_ < 3)
            parent_table_virt[i] |= PD_RDONLY;
            // counter_++;
        }
        else {
            if (!child_table_virt[i]){
                size_t *new_table = kmalloc(0x1000);
                memset(new_table, 0, 0x1000);
                child_table_virt[i] = VIRT_TO_PHYS((size_t)new_table); 
                child_table_virt[i] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
            }
            size_t *parent_next_table = (size_t*)(parent_table_virt[i] & ENTRY_ADDR_MASK);
            size_t *child_next_table = (size_t*)(child_table_virt[i] & ENTRY_ADDR_MASK);
            mmu_reset_page_tables_read_only(parent_next_table, child_next_table, level + 1);
        }
    }
    unlock();
}

/*
    Get the vma by virtual address
    @t: the thread
    @va: the virtual address
*/
vm_area_struct_t *get_vma_by_va(thread_t *t, size_t va)
{
    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *target_vma = 0;
    int counter = 0;
    list_for_each(pos, &t->vma_list)
    {
        vma = (vm_area_struct_t *)pos;
        if (vma->virt_addr <= va && vma->virt_addr + vma->area_size > va)
        {
            target_vma = vma;
        }
        else
        counter++;
    }

    // show_vma_list((int[]){target_counter}, 1);
    return target_vma;
}

/*
    Get the vma by the counter
    @t: the thread
    @counter: the counter
*/
void show_vma_list(int *highlight_array, int size)
{
    list_head_t *pos;
// #ifdef DEBUG
// vm_area_struct_t *vma;
// #endif
    #ifdef DEBUG
    vm_area_struct_t *vma;
    #endif

    int counter = 0;
    int hightlight_idx = 0;
    // frame_t *frame_array = get_frame_array();
    
    mmu_sendline("     ================================================================ VMA List ================================================================\n");
    list_for_each(pos, &curr_thread->vma_list)
    {
        #ifdef DEBUG
        vma = (vm_area_struct_t *)pos;
        #endif

        if (hightlight_idx < size && highlight_array[hightlight_idx] == counter)
        {
            mmu_sendline(BLU "     (%d) %s , virt_addr: 0x%12x, phys_addr: 0x%12x, area_size: 0x%7x, prot: %d (%s)\r\n" RESET, 
                counter, 
                vma->name,
                // frame_counter, 
                vma->virt_addr, 
                vma->phys_addr, 
                vma->area_size, 
                vma->rwx,
                VMA_prot_table[vma->rwx]
            );
            hightlight_idx++;
        }
        else{

            mmu_sendline("     (%d) %s , virt_addr: 0x%12x, phys_addr: 0x%12x, area_size: 0x%7x, prot: %d (%s)\r\n", 
                counter, 
                vma->name, 
                vma->virt_addr, 
                vma->phys_addr, 
                vma->area_size, 
                vma->rwx, 
                VMA_prot_table[vma->rwx]
            );
        }
        counter++;
    }

    mmu_sendline("     ==========================================================================================================================================\n");

}

void handle_segmentation_fault()
{
    uart_sendline(RED "[Segmentation fault (VMA not found)] kill Process\r\n" RESET);
    uart_recv_echo_flag = 1; 

    thread_exit();
}

void handle_translation_fault(size_t error_addr, vm_area_struct_t *vma_area_ptr)
{
    size_t addr_offset = (error_addr - vma_area_ptr->virt_addr);
    addr_offset = (addr_offset % 0x1000) == 0 ? addr_offset : addr_offset - (addr_offset % 0x1000);

    size_t flag = 0;
    if(!(vma_area_ptr->rwx & (0b1 << 2))) flag |= PD_UNX;        // 4: executable
    if(!(vma_area_ptr->rwx & (0b1 << 1))) flag |= PD_RDONLY;     // 2: writable
    if(  vma_area_ptr->rwx & (0b1 << 0) ) flag |= PD_UK_ACCESS;  // 1: readable / accessible
    map_one_page(
        (size_t *)PHYS_TO_VIRT(curr_thread->context.pgd), 
        vma_area_ptr->virt_addr + addr_offset, 
        vma_area_ptr->phys_addr + addr_offset, 
        flag
    );

    mmu_sendline(GRN "[Translation fault fixed]: Update Page Table\r\n" RESET); // far_el1: Fault address register.
}

/*
    Translate the virtual address to physical address by the page table
    @param virt_pgd_p: the pointer of the page table
    @param va: the virtual address
*/
size_t virt_to_phys_paging_table(size_t *virt_pgd_p, size_t va)
{
    size_t *table_p = virt_pgd_p;
    for (int level = 0; level < 4; level++)
    {
        unsigned int idx = (va >> (39 - level * 9)) & 0x1ff; // p.14, 9-bit only
        if (level == 3)
        {
            return (table_p[idx] & ENTRY_ADDR_MASK) | (va & 0xfff);
        }

        if(!table_p[idx])
        {
            return 0;
        }

        table_p = (size_t*)PHYS_TO_VIRT((size_t)(table_p[idx] & ENTRY_ADDR_MASK)); // PAGE_SIZE
    }
    return 0;
}
void handle_permission_fault(size_t error_addr, vm_area_struct_t *vma_area_ptr)
{
    // vma not writable call handle_translation_fault
    if ( !(vma_area_ptr->rwx & (0b1 << 2))){
        uart_sendline(RED "The vma is not writable\r\n" RESET);
        uart_recv_echo_flag = 1; 
        unlock();
        thread_exit();
    }

    frame_t *frame_array = get_frame_array();
    size_t phy_error_addr = virt_to_phys_paging_table( (unsigned long*)PHYS_TO_VIRT(curr_thread->context.pgd), error_addr);
    int error_frame_idx = PTR_TO_PAGE_INDEX(PHYS_TO_VIRT(phy_error_addr));

    frame_t *error_frame = &frame_array[error_frame_idx];

    // the frame counter is 1, so there is only one process use the page frame and handle translation fault
    if (error_frame->counter == 1){
        handle_translation_fault(error_addr, vma_area_ptr);
        return;
    }


    // handle copy-on-write
    mmu_sendline(GRN "[Handle Copy-on-Write]\r\n" RESET);

    // change the counter of frame
    unsigned long long curr_offset = 0;
    int frame_idx;
    while(curr_offset <= vma_area_ptr->area_size){
        frame_idx = PTR_TO_PAGE_INDEX(vma_area_ptr->phys_addr + curr_offset);
        frame_array[frame_idx].counter--;
        curr_offset += 0x1000;
    }
    
    // We new a frame and copy the content of the old frame to the new frame
    void *new_frame = kmalloc(0x1000);
    memcpy(new_frame, (void*)(error_addr & ENTRY_ADDR_MASK), 0x1000);


    int prev_vma_list_size = list_size(&curr_thread->vma_list);

    // delete current vma
    list_del_entry(&vma_area_ptr->listhead);

    // add the left vma
    mmu_add_vma(
        curr_thread,                                                                                        // thread
        vma_area_ptr->virt_addr,                                                                            // virt_addr
        (error_addr & ENTRY_ADDR_MASK) - vma_area_ptr->virt_addr,                                           // size
        vma_area_ptr->phys_addr,                                                                            // phys_addr                
        vma_area_ptr->rwx,                                                                                  // rwx
        vma_area_ptr->name,                                                                                 // name
        1                                                                                                   // is_alloced
    );

    // add the center vma, it's also the error vma
    mmu_add_vma(
        curr_thread,                                                                                        // thread
        error_addr  & ENTRY_ADDR_MASK,                                                                      // virt_addr
        0x1000,                                                                                             // size
        VIRT_TO_PHYS((size_t)new_frame),                                                                    // phys_addr
        vma_area_ptr->rwx,                                                                                  // rwx
        vma_area_ptr->name,                                                                                 // name
        1                                                                                                   // is_alloced
    );
    handle_translation_fault(error_addr, (vm_area_struct_t*) curr_thread->vma_list.prev);

    // add the right vma
    mmu_add_vma(
        curr_thread,                                                                                        // thread
        (error_addr & ENTRY_ADDR_MASK) + 0x1000,                                                            // virt_addr
        vma_area_ptr->area_size - ((error_addr & ENTRY_ADDR_MASK) - vma_area_ptr->virt_addr) - 0x1000,      // size
        vma_area_ptr->phys_addr + ((error_addr & ENTRY_ADDR_MASK) - vma_area_ptr->virt_addr) + 0x1000,      // phys_addr
        vma_area_ptr->rwx,                                                                                  // rwx
        vma_area_ptr->name,                                                                                 // name
        1                                                                                                   // is_alloced
    );


    // show the vma list with highlight
    mmu_sendline(GRN "[Copy-on-Write]: VMA List Updated\r\n" RESET);
    int vma_list_size = list_size(&curr_thread->vma_list);
    int highlight_head = prev_vma_list_size - 1;
    int hightlight_size = vma_list_size - prev_vma_list_size + 1;
    int highlight_array[3];
    for (int i = 0; i < hightlight_size; i++)
    {
        highlight_array[i] = highlight_head + i;
    }
    show_vma_list(highlight_array, hightlight_size);
}


void mmu_memfail_abort_handle(esr_el1_t* esr_el1)
{
    mmu_sendline("\n---------------------------------------------------- mmu_memfail_abort_handle ----------------------------------------------------\r\n");
    unsigned long long far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t": "=r"(far_el1));

    vm_area_struct_t *vma_area_ptr = 0;

    #ifdef DEBUG
    char *IFSC_error_message = IFSC_table[(esr_el1->iss & 0x3f)];
    #endif
    vma_area_ptr = get_vma_by_va(curr_thread, far_el1);
    


    // (Segmentation Fault)area is not part of process's address space
    if (!vma_area_ptr)
    {
        mmu_sendline(RED "[!][Memory fault]: 0x%x" YEL " (Error Message: %s)\r\n" RESET,far_el1, IFSC_error_message); // far_el1: Fault address register.
        mmu_sendline(RED "[!]" CYN "[Pid: %d]" RED "[Segmentation fault]: Kill Process\r\n" RESET, curr_thread->pid);
        handle_segmentation_fault();
    }

    // (translation fault) only map one page frame for the fault address
    else if (
        (esr_el1->iss & 0x3f) == TF_LEVEL0 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL1 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL2 ||
        (esr_el1->iss & 0x3f) == TF_LEVEL3)
    {
        lock();
        mmu_sendline(RED "[!]" CYN "[Pid %d]" RED "[translation fault]: 0x%x" RESET " (Error Message: %s)\r\n" RESET,curr_thread->pid, far_el1, IFSC_error_message); // far_el1: Fault address register.
        // show_vma_list((int[]){find_list_entry(&curr_thread->vma_list, (list_head_t*)vma_area_ptr)}, 1);
        handle_translation_fault(far_el1, vma_area_ptr);
        unlock();
    }
    // (Permission fault)
    else if (
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL0 ||
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL1 ||
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL2 ||
        (esr_el1->iss & 0x3f) == PERMISSON_FAULT_LEVEL3
    )
    {
        lock();
        mmu_sendline(RED "[!]" CYN "[Pid %d]" RED "[Permission fault]: 0x%x" YEL " (Error Message: %s)\r\n" RESET,curr_thread->pid, far_el1, IFSC_error_message); // far_el1: Fault address register.
        show_vma_list((int[]){find_list_entry(&curr_thread->vma_list, (list_head_t*)vma_area_ptr)}, 1);
        handle_permission_fault(far_el1, vma_area_ptr);

        unlock();
    }
    else
    {
        mmu_sendline(RED "[!]" CYN "[Pid: %d]" RED "[Segmentation fault]: Kill Process\r\n" RESET, curr_thread->pid);
        uart_recv_echo_flag = 1; 
        thread_exit();
    }
}
