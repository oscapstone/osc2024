
#include "mmu.h"
#include "utils/utils.h"
#include "mm/mm.h"
#include "arm/sysregs.h"

void setup_identity_mapping()
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
    memset(PD_KERNEL_ENTRY, 0, 0x1000);
    memset(PD_FIRST_PUD_ENTRY, 0, 0x1000);

    U64* table_ptr = (U64*)PD_KERNEL_ENTRY;
    *table_ptr = (PD_KERNEL_PGD_ATTR | PD_FIRST_PUD_ENTRY);  // the address of PUD
    table_ptr = (U64*)PD_FIRST_PUD_ENTRY;
    // map 0
    *table_ptr = (PD_KERNEL_PUD_ATTR | 0);
    table_ptr += 1;
    *table_ptr = (PD_KERNEL_PUD_ATTR | 0x40000000);

    utils_write_sysreg(ttbr0_el1, (U64*)0x1000);
    utils_write_sysreg(ttbr1_el1, (U64*)0x1000);

    // enabling MMU
    U64 sctlr = utils_read_sysreg(sctlr_el1);
    utils_write_sysreg(sctlr_el1, sctlr | SCTLR_MMU_ENABLED);
}