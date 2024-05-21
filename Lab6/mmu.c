#include "mmu_regs.h"
#include "memory.h"
#define DEVICE_BASE 0x3F000000L 

void three_level_translation_init(){
    unsigned long *pmd_1 = (unsigned long *) 0x3000;
    for(unsigned long i=0; i<512; i++){
        unsigned long base = 0x200000L * i; // 2 * 16^5 -> 2MB
        if(base >= DEVICE_BASE){ 
            //map as device
            pmd_1[i] = PD_ACCESS + PD_BLOCK + base + (MAIR_IDX_DEVICE_nGnRnE << 2) + PD_KERNEL_USER_ACCESS;
        }
        else{
            //map as normal
            pmd_1[i] = PD_ACCESS + PD_BLOCK + base + (MAIR_IDX_NORMAL_NOCACHE << 2);
        }
    }

    unsigned long *pmd_2 = (unsigned long *) 0x4000;
    for(unsigned long i=0; i<512; i++){
        unsigned long base = 0x40000000L + 0x200000L * i;
        pmd_2[i] = PD_ACCESS + PD_BLOCK + base + (MAIR_IDX_NORMAL_NOCACHE << 2);
    }

    unsigned long * pud = (unsigned long *) 0x2000;
    *pud = PD_ACCESS + (MAIR_IDX_NORMAL_NOCACHE << 2) + PD_TABLE + (unsigned long) pmd_1;
    pud[1] = PD_ACCESS + (MAIR_IDX_NORMAL_NOCACHE << 2) + PD_TABLE + (unsigned long) pmd_2;
}

//va: virtual address, pa: physical address
void walk(unsigned long * pgd, unsigned long va, unsigned long pa){
    for(int i=0; i<4; i++){
        unsigned int offset = (va >> (39 - i * 9)) & 0x1ff;
        
        if(i == 3){ //set the physical address
            pgd[offset] = pa;
            pgd[offset] |=  PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KERNEL_USER_ACCESS;
            return;
        }
        
        //ensure memset 0 first
        if(pgd[offset] == 0){
            //assign a new page
            unsigned long * new_page_table = allocate_page(4096);
            for(int j=0; j<4096; j++){
                ((char *) (new_page_table))[j] = 0;
            }
            pgd[offset] = new_page_table - VT_OFFSET; // to physical
            pgd[offset] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KERNEL_USER_ACCESS;
        }

        pgd = (unsigned long * ) (((unsigned long) (pgd[offset] & ENTRY_ADDR_MASK)) + VT_OFFSET);
    }
}

void map_pages(unsigned long * pgd, unsigned long va, unsigned long size, unsigned long pa){
    for(unsigned long i = 0; i < size; i+=4096){
        walk(pgd, va + i, pa + i);
    }
}

unsigned long ensure_virtual(unsigned long addr){
    if(addr < VT_OFFSET)
        addr += VT_OFFSET;
    return addr;
}