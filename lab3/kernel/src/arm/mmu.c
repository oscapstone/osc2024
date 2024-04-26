
#include "mmu.h"
#include "utils/utils.h"
#include "mm/mm.h"
#include "arm/sysregs.h"

void setup_kernel_page_table()
{
    // set the default translation config register
    utils_write_sysreg(tcr_el1, TCR_CONFIG_DEFAULT);

    // set the default memory attribute indirection register
    utils_write_sysreg(mair_el1, MAIR_DEFAULT_VALUE);

    // our page table look like this 2 frame
    // first frame PGD frame
    // | PGD
    // second frame PUD frame
    // | PUD kernel memory
    // | PUD device memory (ARM peripherals)
    memset(0x1000, 0, 0x1000);
    memset(0x2000, 0, 0x1000);

    U64* table_ptr = (U64*)0x1000;
    *table_ptr = (PD_KERNEL_PGD_ATTR | 0x2000);  // the address of PUD
    table_ptr = (U64*)0x2000;
    *table_ptr = (PD_KERNEL_PUD_ATTR | 0);
    table_ptr += 1;
    *table_ptr = (PD_KERNEL_PUD_ATTR | 0x40000000);

    utils_write_sysreg(ttbr0_el1, (U64*)0x1000);
    utils_write_sysreg(ttbr1_el1, (U64*)0x1000);

    // enabling MMU
    U64 sctlr = utils_read_sysreg(sctlr_el1);
    utils_write_sysreg(sctlr_el1, sctlr | SCTLR_MMU_ENABLED);
}

void create_pgd_entry(U64 tbl, U64 v_addr) {
    create_table_entry(tbl, v_addr, PD_PGD_SHIFT);
    tbl += PD_PAGE_SIZE;
    create_table_entry(tbl, v_addr, PD_PUD_SHIFT);
}

/**
 * @param tbl
 *      pointer to this table entry physical address
 * @param shift
 *      the shift of the virtual address,
 *      PD_PGD_SHIFT, PD_PUF_SHIFT
 * 
*/
void create_table_entry(U64 tbl, U64 v_addr, U64 shift) {
    U64 shift_addr = v_addr << shift;
    shift_addr &= (PD_PTRS_PER_TABLE - 1);
    U64 next_tbl = tbl + PD_PAGE_SIZE;

}

