#ifndef VM_H
#define VM_H

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE      0b00000000
#define MAIR_NORMAL_NOCACHE     0b01000100
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1
#define MAIR_CONFIG_DEFAULT                                 \
    ((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
     (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))

#define PD_TABLE       0b11
#define PD_BLOCK       0b01
#define PD_ENTRY       0b11
#define PD_UK_ACCESS   (1 << 6)
#define PD_RDONLY      (1 << 7)
#define PD_ACCESS      (1 << 10)
#define PD_NORMAL_ATTR (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK)
#define PD_DEVICE_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define PE_NORMAL_ATTR \
    (PD_ACCESS | PD_UK_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_ENTRY)

#define PHYS_TO_VIRT(x) (x + 0xFFFF000000000000)
#define VIRT_TO_PHYS(x) (x - 0xFFFF000000000000)

#ifndef __ASSEMBLER__

// #define PROT_NONE  0
// #define PROT_READ  1
// #define PROT_WRITE 2
// #define PROT_EXEC  4

// #define MAP_ANONYMOUS 0x0020
// #define MAP_POPULATE  0x4000

struct vm_area_struct {
    unsigned long vm_start;
    unsigned long vm_end;
    unsigned long vm_page_prot;
    struct vm_area_struct *vm_prev;
    struct vm_area_struct *vm_next;
};

void map_pages(unsigned long pgd, unsigned long va, unsigned long size,
               unsigned long pa, unsigned long prot);

#endif // __ASSEMBLER__

#endif // VM_H
