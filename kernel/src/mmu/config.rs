// pub const PAGE_SIZE: usize = 4096;
pub const ENTRY_COUNT: usize = 512;

pub const TCR_CONFIG_REGION_48BIT: u64 = (64 - 48) << 0 | (64 - 48) << 16;
pub const TCR_CONFIG_REGION_4KB: u64 = 0b00 << 14 | 0b00 << 30;
pub const TCR_CONFIG_DEFAULT: u64 =
    TCR_CONFIG_REGION_48BIT | TCR_CONFIG_REGION_4KB | 0b101u64 << 32;

pub const MAIR_DEVICE_NG_NR_NE: u8 = 0b00000000;
pub const MAIR_NORMAL_NC: u8 = 0b01000100;
pub const MAIR_DEVICE_NG_NR_NE_IDX: u8 = 0;
pub const MAIR_NORMAL_NC_IDX: u8 = 1;
pub const MAIR_CONFIG_DEFAULT: u64 = (MAIR_DEVICE_NG_NR_NE as u64)
    << (MAIR_DEVICE_NG_NR_NE_IDX * 8)
    | (MAIR_NORMAL_NC as u64) << (MAIR_NORMAL_NC_IDX * 8);

pub const L0_ADDR: u64 = 0x1000;
pub const L1_ADDR: u64 = 0x2000;
pub const L2_ADDR: u64 = 0x3000;

pub const PD_TABLE: u32 = 0b11;
pub const PD_BLOCK: u32 = 0b01;
pub const PD_PAGE: u32 = 0b11;
pub const PD_ACCESS: u32 = 1 << 10;

pub const AP_RW_EL0: usize = 0b01 << 6;
pub const AP_RO_EL0: usize = 0b11 << 6;

pub const STACK_CONFIG: u32 =
    PD_ACCESS | AP_RW_EL0 as u32 | (MAIR_NORMAL_NC_IDX as u32) << 2 as u32 | PD_PAGE as u32;

pub const TEXT_CONFIG: u32 =
    PD_ACCESS | AP_RO_EL0 as u32 | (MAIR_NORMAL_NC_IDX as u32) << 2 as u32 | PD_PAGE as u32;

pub const GPU_CONFIG: u32 =
    PD_ACCESS | AP_RW_EL0 as u32 | (MAIR_DEVICE_NG_NR_NE_IDX as u32) << 2 as u32 | PD_PAGE as u32;
