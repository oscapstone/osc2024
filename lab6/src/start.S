#include <start.h>

.section ".text.boot"

.global _start
_start:
    mov x10, x0 // preserve dtb base address

    /* switch from EL2 to EL1 */
    bl from_el2_to_el1

/* --- lab6 --- */

    /* set up kernel page tables */
    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0 // Translation Control Register

    ldr x0, =( \
        (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | \
        (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)) \
    )
    msr mair_el1, x0 // Memory Attribute Indirection Register

    mov x0, BOOT_PGD_PAGE_FRAME  // PGD's page frame
    mov x1, BOOT_PUD_PAGE_FRAME  // PUD's page frame
    mov x2, BOOT_PMD_PAGE_FRAME  // PMD's page frame

    // PGD
    ldr x6, =BOOT_PGD_ATTR
    orr x6, x1, x6 // set PGD->PUD entries
    str x6, [x0]

    // PUD
    ldr x6, =BOOT_PUD_ATTR
    orr x6, x2, x6 // set PUD[0]->PMD entries
    str x6, [x1]

    ldr x6, =BOOT_PUD_DEVICE_ATTR
    mov x3, 0x40000000
    orr x6, x3, x6 // set PUD[1]->MMIO device addr
    str x6, [x1, 8]

    /*
    +-------------------+ 0x00000000
    | PGD[0]->PUD[0]    |   PGD bottom
    |                   |
    |-------------------| 0x00001000
    | PUD[0]->PMD[0]    |   PUD bottom
    | PUD[1]>>0x40000000|   PUD to MMIO device
    |                   |
    |-------------------| 0x00002000 
    | PMD[0]>>0x0       |   PMD bottom
    | PMD[1]>>0x200000  |   PMD to normal memory
    | PMD[2]>>0x400000  |   ...
    | ...               |
    | PMD[_]>>0x3F000000|   PMD to MMIO device
    | ...               |   ...
    | PMD[_]>>0x3FE00000|   PMD top
    |                   |
    |-------------------| 0x00003000 
    | ...               |

     */


    // PMD
    mov x3, 0x00000000  // PMD block bottom
    mov x4, 0x3F000000  // MMIO device bottom
    mov x5, 0x40000000  // PMD block top

pmd_loop:
    cmp x3, x5          // reach PMD top?
    beq enable_mmu      // leave the loop
    cmp x3, x4          // reach MMIO device bottom?
    ldr x6, =BOOT_PMD_DEVICE_ATTR // yes, set device PMD entry
    bge set_pmd_entry
    ldr x6, =BOOT_PMD_NORMAL_ATTR // no, set normal PMD entry

set_pmd_entry:
    orr x6, x3, x6
    str x6, [x2], #8 // x2 += 8
    add x3, x3, 0x200000
    b pmd_loop

enable_mmu: // mmu: memory management unit
    msr ttbr0_el1, x0 // [no need?]will be modified by each user process
    msr ttbr1_el1, x0 // also load PGD to the upper translation based register
    mrs x2, sctlr_el1 // System Control Register
    orr x2, x2, 1
    msr sctlr_el1, x2 // enable MMU, cache remains disabled

    ldr x2, =boot_rest  // indirect branch to the virtual address
    br x2

/* --- lab6^^^ --- */

boot_rest:
    /* get dtb address from x10 */
    ldr x1, =DTB_BASE // defined in devtree.c
    str x10, [x1]

    /* get cpu id */
    mrs x1, mpidr_el1 /* Multiprocessor Affinity Register */
    and x1, x1, #3
    cbnz x1, halt // halt if cpu id != 0

    /* set exception vector table */
    adr x0, exception_vector_table // defined in traps.S
    msr vbar_el1, x0

    /* set stack pointer */
    ldr x1, =_start
    mov sp, x1

    /* clear bss section */
    ldr x1, =__bss_start
    ldr w2, =__bss_size

bss_reset:
    cbz w2, run_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, bss_reset

run_main:
    /* branch to main function */
    bl main

halt:
    wfe
    b halt

from_el2_to_el1:
    mov x0, (1 << 31)
    msr hcr_el2, x0 // EL1 uses aarch64
    mov x0, 0x3C5   // ... 0011 1100 0101
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1
