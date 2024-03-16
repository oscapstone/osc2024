use tock_registers::{
    register_bitfields, register_structs,
    registers::{ReadOnly, WriteOnly},
};

use crate::common::MMIODerefWrapper;

register_bitfields! {
    u32,

    pub MAILBOX_STATUS [
        FULL OFFSET(31) NUMBITS(1) [
            NotFull = 0,
            Full = 1,
        ],
        EMPTY OFFSET(30) NUMBITS(1) [
            NotEmpty = 0,
            Empty = 1,
        ]
    ],
    pub MAILBOX_READ [
        DATA OFFSET(4) NUMBITS(28) [],
        CHANNEL OFFSET(0) NUMBITS(4) []
    ],
    pub MAILBOX_WRITE [
        DATA OFFSET(4) NUMBITS(28) [],
        CHANNEL OFFSET(0) NUMBITS(4) []
    ]
}

register_structs! {
    #[allow(non_snake_case)]
    pub RegisterBlock {
        (0x00 => pub MAILBOX_READ: ReadOnly<u32, MAILBOX_READ::Register>),
        (0x04 => _reserved1),
        (0x18 => pub MAILBOX_STATUS: ReadOnly<u32, MAILBOX_STATUS::Register>),
        (0x1c => _reserved2),
        (0x20 => pub MAILBOX_WRITE: WriteOnly<u32, MAILBOX_WRITE::Register>),
        (0x24 => @END),
    }
}

pub type Registers = MMIODerefWrapper<RegisterBlock>;
