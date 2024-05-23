# OSC2024 Lab6
## Translation Control Register (TCR)
set tcr_el1 to 0b10000000000100000000000000010000: 指定虛擬記憶體的地址範圍為 48 位，並且頁面大小為 4KB。
[tct_el1](https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/TCR-EL1--Translation-Control-Register--EL1-?lang=en#fieldset_0-5_0)
```
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

ldr x0, = TCR_CONFIG_DEFAULT
msr tcr_el1, x0
```

## Memory Attribute Indirection Register (MAIR)
Set MAIR of device (nGnRnE) and normal memory (bit [4:2] -> 0 or 1)
```
#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

ldr x0, =( \
  (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
  (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
)
msr mair_el1, x0
```

## Identity Paging
### Two-Level Translation
Only PGD and PUD are used.
#### Basic Settings
```
//The label of attribute (next table / memory block)
#define PD_TABLE 0b11
#define PD_BLOCK 0b01
//set the access bit of memory to 1
#define PD_ACCESS (1 << 10)
//attribute of tables
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
//note: simply load all memory to device memory here (must change)
```
#### Set tables
```
mov x0, 0x1000 // PGD's page frame at 0x1000
mov x1, 0x2000 // PUD's page frame at 0x2000

//x1: next PUD addr, x2: next PUD attribute, combine and placed in x0
ldr x2, = BOOT_PGD_ATTR
orr x2, x1, x2 // combine the physical address of next level page with attribute.
str x2, [x0]

//x2: memory block attribute, x3: memory
ldr x2, = BOOT_PUD_ATTR
mov x3, 0x00000000
orr x3, x2, x3
str x3, [x1] // 1st 1GB mapped by the 1st entry of PUD (x1, 0)
mov x3, 0x40000000
orr x3, x2, x3
str x3, [x1, 8] // 2nd 1GB mapped by the 2nd entry of PUD (x1, 8)

//set the PGD for the mmu
msr ttbr0_el1, x0 // bottom -> user space (need to be implemented)
msr ttbr1_el1, x0 // upper -> kernel space

//enable MMU
mrs x2, sctlr_el1
orr x2 , x2, 1
msr sctlr_el1, x2

ldr x2, = boot_rest // indirect branch to the virtual address
br x2
```

### Update Linker
```
//Place kernel space to upper memory
SECTIONS
{
  . = 0xffff000000000000; // kernel space
  . += 0x80000; // kernel load address
  _kernel_start = . ;
  // ...
}
```

### Finer Granularity Paging
* Set PMD for each PGD entry which then find a block with smaller memory (2MB) (three-level translation).
* Map peripheral (MMIO_BASE: 0x3F000000) to device memory by setting MAIR_IDX.
```
void three_level_translation_init(){
    // bit 47~22 for physical address, and 21 for offset in last level
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
    pud[0] = PD_ACCESS + (MAIR_IDX_NORMAL_NOCACHE << 2) + PD_TABLE + (unsigned long) pmd_1;
    pud[1] = PD_ACCESS + (MAIR_IDX_NORMAL_NOCACHE << 2) + PD_TABLE + (unsigned long) pmd_2;
}
```
#### Peripheral Address: 0x3F000000 ~ 0x3FFFFFFF
(p6) https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

### Context Switch
```
ldr x0, = next_pgd //set next pgd
dsb ish
//Data Synchronization Barrier, ensure write finish
//ish（Inner Shareable）sharable memory

msr ttbr0_el1, x0

tlbi vmalle1is//el1 tlbs clear (cache for memory access)
//TLB Invalidate
//vmalle1is: EL1 TLBs

dsb ish

isb //Instruction Synchronization Barrier: instruction complete

```

### MAIR 
setting of different memory (just google mair-el1  if cant open)

* https://developer.arm.com/documentation/ddi0595/2021-06/AArch32-Registers/MAIR1--Memory-Attribute-Indirection-Register-1
* Device Memory: https://developer.arm.com/documentation/den0024/latest/Memory-Ordering/Memory-types/Device-memory

nGnRnE: access gather(match),access order, signal write

### Note
* TTBR Res0: 因為一個 page 是 4096 (2^12)，要 align
* 找地址: 前 16 高低位， 48 開始每次 9，最後 12 是 offset (2^12)
* Entry mask: 最後 12 bit (for) settings
* init 的 n : 2MB x 512 = 1 GB, 2MB = 2 ^ 21 -> n = 20
* 1GB -> 31:30 0 or 1 -> PMD (2 ^ 31)


## 問 
* allocate_page 問題
* 最後一層 BLOCK vs TABLE (SPEC)
* init 不能都給 PD_KERNEL_USER_ACCESS
