use core::arch::asm;
pub mod config;
mod entry;
mod page_table;
pub mod vm;

use crate::mmu::config::L0_ADDR;
use crate::mmu::config::L1_ADDR;
use crate::mmu::config::L2_ADDR;
use crate::mmu::config::MAIR_CONFIG_DEFAULT;
use crate::mmu::config::MAIR_DEVICE_NG_NR_NE_IDX;
use crate::mmu::config::MAIR_NORMAL_NC_IDX;
use crate::mmu::config::PD_ACCESS;
use crate::mmu::config::PD_BLOCK;
use crate::mmu::config::PD_TABLE;
use crate::mmu::config::TCR_CONFIG_DEFAULT;

#[no_mangle]
unsafe extern "C" fn set_mmu() {
    asm!(
        "msr tcr_el1, {0}",
        "msr mair_el1, {1}",
        in(reg) TCR_CONFIG_DEFAULT,
        in(reg) MAIR_CONFIG_DEFAULT,
    );

    // Set up PGD
    // 0b0000_0000_AAAA_AAAA_ABBB_BBBB_BBCC_CCCC_CCCD_DDDD_DDDD_XXXX_XXXX_XXXX
    //             0000_0000_0
    *(L0_ADDR as *mut u64) = L1_ADDR | PD_TABLE as u64;

    // Set up PUD
    // 0b0000_0000_AAAA_AAAA_ABBB_BBBB_BBCC_CCCC_CCCD_DDDD_DDDD_XXXX_XXXX_XXXX
    //                        000_0000_00
    *(L1_ADDR as *mut u64) = L2_ADDR | PD_TABLE as u64;

    // Set up PMD
    // 0b0000_0000_AAAA_AAAA_ABBB_BBBB_BBCC_CCCC_CCCD_DDDD_DDDD_XXXX_XXXX_XXXX
    //                                   00_0000_000
    for i in (0x0000_0000 / (1 << 9) / (1 << 12))..(0x3C00_0000 / (1 << 9) / (1 << 12)) {
        let addr = L2_ADDR + i * 8;
        let attr: u64 = PD_ACCESS as u64 | (MAIR_NORMAL_NC_IDX as u64) << 2 | PD_BLOCK as u64;
        *(addr as *mut u64) = attr | (i * (1 << 9) * (1 << 12));
    }
    for i in (0x3C00_0000 / (1 << 9) / (1 << 12))..(0x4000_0000 / (1 << 9) / (1 << 12)) {
        let addr = L2_ADDR + i * 8;
        let attr: u64 = PD_ACCESS as u64 | (MAIR_DEVICE_NG_NR_NE_IDX as u64) << 2 | PD_BLOCK as u64;
        *(addr as *mut u64) = attr | i * (1 << 9) * (1 << 12);
    }

    *(L1_ADDR.wrapping_add(8) as *mut u64) = 0x4000_0000 as u64
        | PD_ACCESS as u64
        | (MAIR_DEVICE_NG_NR_NE_IDX as u64) << 2
        | PD_BLOCK as u64;

    asm!(
        "msr ttbr0_el1, {l0}",
        "msr ttbr1_el1, {l0}",
        "isb",
        l0 = in(reg) L0_ADDR,
    );

    asm!(
        "mrs {0}, sctlr_el1",
        "orr {0}, {0}, #1",
        "msr sctlr_el1, {0}",
        "isb",
        out(reg) _,
    )
    // 0x00c50838
    // 0b0000_0000_1100_0101_0000_1000_0011_1000
}
