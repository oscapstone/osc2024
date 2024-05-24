#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_PAGE 0b11
#define PD_ACCESS (1 << 10)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR PD_TABLE
#define BOOT_PMD_ATTR PD_TABLE
#define BOOT_PTE_NORMAL_ATTR (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE)
#define BOOT_PTE_DEVICE_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_PAGE)

.section ".text.boot"

.global dtb_ptr

.global _start
.global halt

_start:
	// read cpu id and stop slave cores
	mrs x1, mpidr_el1
	and x1, x1 ,#3
	cbz x1, _from_el2_to_el1

halt:
	wfe
	b halt

_from_el2_to_el1:
	bl from_el2_to_el1
	// move dtb to x14
	mov x14, x0

_set_exception_vector_table:
	// initialize exceptions
  	adr x1, exception_vector_table
	orr x1, x1, 0xFFFF000000000000
  	msr vbar_el1, x1

_set_mmu_config:
	// set up translation control
	ldr x0, = TCR_CONFIG_DEFAULT
    msr tcr_el1, x0
	// set up memory attribute
	ldr x0, =( \
      (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
      (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
    )
    msr mair_el1, x0
	bl _create_page_tables

_enable_mmu:
	mrs x2, sctlr_el1
    orr x2 , x2, 1
    msr sctlr_el1, x2 // enable MMU, cache remains disabled

	ldr x2, = set_sp // indirect branch to the virtual address
	br x2

set_sp:
	// set stack pointer to the _start
	msr ttbr0_el1, x14 // to test kernel mapping is correct
	ldr x1, =_start
	mov sp, x1

	// clear bss
	ldr x1, =__bss_start
	ldr x2, =__bss_size

clear_bss:
	cbz x2, jump_to_main
	str xzr, [x1], #8
	sub x2, x2, #1
	cbnz x2, clear_bss

jump_to_main:
	// put address of dtb into dtb_ptr
	ldr x1, =dtb_ptr
	orr x14, x14, 0xFFFF000000000000
	str x14, [x1]

	b main
	b halt

from_el2_to_el1:
    mov x1, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x1
    mov x1, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x1
    msr elr_el2, lr
	ldr x1, =_start
	msr sp_el1, x1 // set sp_ep1
    eret // return to EL1

_create_page_tables:
	// PGD's page frame at pg_dir in physical address
	ldr x0, =pg_dir
	lsl x0, x0, 16
    lsr x0, x0, 16

	// PUD page table mapped by the 1st entry of PGD
	add x1, x0, 0x1000
    ldr x4, = BOOT_PGD_ATTR
    orr x4, x1, x4
    str x4, [x0]
	// 1st PMD page table mapped by the 1st entry of PUD
	add x2, x1, 0x1000
    ldr x4, = BOOT_PUD_ATTR
    orr x4, x2, x4
    str x4, [x1]
	 // 2nd PMD page table mapped by the 2nd entry of PUD
	add x3, x2, 0x1000
    ldr x4, = BOOT_PUD_ATTR
    orr x4, x3, x4
    str x4, [x1, 8]

	add x4, x3, 0x1000 // save 1st PTE page table to x4 
	mov x12, x4
	mov x10, xzr
	mov x11, 512
_init_1st_PMD_entry:
	ldr x5, = BOOT_PMD_ATTR
	orr x5, x4, x5
	str x5, [x2], 8 // mapped by the entry of the 1st PMD table and change to next entry
	add x4, x4, 0x1000 // change to next PTE page table
	add x10, x10, 1
	cmp x10, x11
	bne _init_1st_PMD_entry

	mov x10, xzr
	mov x11, 512
_init_2nd_PMD_entry:
	ldr x5, = BOOT_PMD_ATTR
	orr x5, x4, x5
	str x5, [x3], 8 // mapped by the entry of the 2nd PMD table and change to next entry
	add x4, x4, 0x1000 // change to next PTE page table
	add x10, x10, 1
	cmp x10, x11
	bne _init_2nd_PMD_entry

	mov x4, x12 // move 1st PTE page table from x12 to x4
	mov x10, xzr // save the first page address to x10
	mov x11, 0x3F000000
_init_PTE_NORMAL_entry:
	ldr x6, = BOOT_PTE_NORMAL_ATTR
	orr x6, x10, x6
	str x6, [x4], 8 // mapped by the entry of the PTE table and change to next entry
	add x10, x10, 0x1000 // change to next page
	cmp x10, x11
	bne _init_PTE_NORMAL_entry

	mov x11, 0x80000000
_init_PTE_DEVICE_entry:
	ldr x6, = BOOT_PTE_DEVICE_ATTR
	orr x6, x10, x6
	str x6, [x4], 8 // mapped by the entry of the PTE table and change to next entry
	add x10, x10, 0x1000 // change to next page
	cmp x10, x11
	bne _init_PTE_DEVICE_entry

    msr ttbr0_el1, x0 // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x0 // load PGD to the upper translation based register.
	
    ret