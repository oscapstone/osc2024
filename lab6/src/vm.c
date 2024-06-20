#include "vm.h"
#include "mm.h"
#include "string.h"

static void walk(unsigned long pt, unsigned long va, unsigned long pa,
                 unsigned long prot)
{
    unsigned long *table = (unsigned long *)pt;
    for (int level = 0; level <= 3; level++) {
        unsigned long offset = (va >> (39 - 9 * level)) & 0x1FF;
        if (level == 3) {
            table[offset] = pa | PE_NORMAL_ATTR;
            return;
        }
        if (!table[offset]) {
            unsigned long *t = kmalloc(PAGE_SIZE);
            memset(t, 0, PAGE_SIZE);
            table[offset] = VIRT_TO_PHYS((unsigned long)t) | PD_TABLE;
        }
        table = (unsigned long *)PHYS_TO_VIRT((table[offset] & ~0xFFF));
    }
}

void map_pages(unsigned long pgd, unsigned long va, unsigned long size,
               unsigned long pa, unsigned long prot)
{
    for (int i = 0; i < size; i += PAGE_SIZE)
        walk(pgd, va + i, pa + i, prot);
}
