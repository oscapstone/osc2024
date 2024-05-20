#include "mmu.h"
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