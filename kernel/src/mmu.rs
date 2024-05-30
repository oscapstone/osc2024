use core::arch::asm;

const TCR_CONFIG_REGION_48BIT: u64 = (64 - 48) << 0 | (64 - 48) << 16;
const TCR_CONFIG_REGION_4KB: u64 = 0b00 << 14 | 0b00 << 30;
const TCR_CONFIG_DEFAULT: u64 = TCR_CONFIG_REGION_48BIT | TCR_CONFIG_REGION_4KB;

const MAIR_DEVICE_NG_NR_NE: u8 = 0b00000000;
const MAIR_NORMAL_NC: u8 = 0b01000100;
const MAIR_DEVICE_NG_NR_NE_IDX: u8 = 0;
const MAIR_NORMAL_NC_IDX: u8 = 1;
const MAIR_CONFIG_DEFAULT: u64 = (MAIR_DEVICE_NG_NR_NE as u64) << (MAIR_DEVICE_NG_NR_NE_IDX * 8)
    | (MAIR_NORMAL_NC as u64) << (MAIR_NORMAL_NC_IDX * 8);

const L0_ADDR: u64 = 0x1000;
const L1_ADDR: u64 = 0x2000;
const L2_ADDR: u64 = 0x3000;
const L3_ADDR: u64 = 0x4000;

const PD_TABLE: u32 = 0b11;
const PD_BLOCK: u32 = 0b01;
const PD_ACCESS: u32 = 1 << 10;

#[no_mangle]
pub unsafe extern "C" fn set_mmu() {
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
    // *(L1_ADDR as *mut u64) =
    //     0x0000_0000 as u64 | PD_ACCESS as u64 | (MAIR_NORMAL_NC_IDX as u64) << 2 | PD_BLOCK as u64;
    // *(L1_ADDR.wrapping_add(8) as *mut u64) = PD_ACCESS as u64
    //     | PD_ACCESS as u64
    //     | (MAIR_DEVICE_NG_NR_NE_IDX as u64) << 2
    //     | PD_BLOCK as u64;

    // Set up PMD
    // 0b0000_0000_AAAA_AAAA_ABBB_BBBB_BBCC_CCCC_CCCD_DDDD_DDDD_XXXX_XXXX_XXXX
    //                                   00_0000_000
    for i in (0x0000_0000 / (1 << 12) / (1 << 9))..(0x3C00_0000 / (1 << 12) / (1 << 9)) {
        let addr = L2_ADDR + i * 8;
        let attr: u64 = PD_ACCESS as u64 | (MAIR_NORMAL_NC_IDX as u64) << 2 | PD_BLOCK as u64;
        *(addr as *mut u64) = attr | (i * (1 << 12) * (1 << 9));
    }
    for i in (0x3C00_0000 / (1 << 12) / (1 << 9))..(0x4000_0000 / (1 << 12) / (1 << 9)) {
        let addr = L2_ADDR + i * 8;
        let attr: u64 = PD_ACCESS as u64 | (MAIR_DEVICE_NG_NR_NE_IDX as u64) << 2 | PD_BLOCK as u64;
        *(addr as *mut u64) = attr | i * (1 << 12) * (1 << 9);
    }

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
}
