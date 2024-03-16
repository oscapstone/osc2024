use tock_registers::{register_structs, registers::ReadWrite};

use crate::common::MMIODerefWrapper;

register_structs! {
    #[allow(non_snake_case)]
    pub RegisterBlock {
        (0x00 => _reserved1),
        (0x1c => pub PM_RSTC: ReadWrite<u32>),
        (0x20 => _reserved2),
        (0x24 => pub PM_WDOG: ReadWrite<u32>),
        (0x28 => @END),
    }
}

pub type Registers = MMIODerefWrapper<RegisterBlock>;
