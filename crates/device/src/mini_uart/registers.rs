use tock_registers::{
    register_bitfields, register_structs,
    registers::{ReadOnly, ReadWrite},
};

use crate::common::MMIODerefWrapper;

register_bitfields! {
    u32,

    pub AUXENB  [
        SPI2 OFFSET(2) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        SPI1 OFFSET(1) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        MINI_UART OFFSET(0) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ]
    ],
    pub AUX_MU_IER [
        RECEIVE_INTERRUPT OFFSET(1) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        TRANSMIT_INTERRUPT OFFSET(0) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ]
    ],
    pub AUX_MU_IIR [
        FIFO_CLEAR_BITS OFFSET(1) NUMBITS(2) [
            ClearReceiveFifo = 0b01,
            ClearTransmitFifo = 0b10,
        ],
    ],
    pub AUX_MU_LCR [
        DATA_SIZE OFFSET(0) NUMBITS(1) [
            SevenBits = 0,
            EightBits = 1,
        ]
    ],
    pub AUX_MU_MCR [
        RTS OFFSET(1) NUMBITS(1) [
            High = 0,
            Low = 1,
        ]
    ],
    pub AUX_MU_LSR [
        TRANSMITTER_IDLE OFFSET(6) NUMBITS(1) [
            Idle = 1,
        ],
        TRANSMITTER_EMPTY OFFSET(5) NUMBITS(1) [
            Empty = 1,
        ],
        RECEIVER_OVERRUN OFFSET(1) NUMBITS(1) [
            Overrun = 1,
        ],
        DATA_READY OFFSET(0) NUMBITS(1) [
            Ready = 1,
        ]
    ],
    pub AUX_MU_CNTL [
        TRANSMIT_AUTO_FLOW_CONTROL OFFSET(3) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        RECEIVE_AUTO_FLOW_CONTROL OFFSET(2) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        TRANSMITTER OFFSET(1) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ],
        RECEIVER OFFSET(0) NUMBITS(1) [
            Disable = 0,
            Enable = 1,
        ]
    ]
}

register_structs! {
    #[allow(non_snake_case)]
    pub RegisterBlock {
        (0x00 => _reserved1),
        (0x04 => pub AUXENB: ReadWrite<u32, AUXENB::Register>),
        (0x08 => _reserved2),
        (0x40 => pub AUX_MU_IO: ReadWrite<u32>),
        (0x44 => pub AUX_MU_IER: ReadWrite<u32, AUX_MU_IER::Register>),
        (0x48 => pub AUX_MU_IIR: ReadWrite<u32, AUX_MU_IIR::Register>),
        (0x4c => pub AUX_MU_LCR: ReadWrite<u32, AUX_MU_LCR::Register>),
        (0x50 => pub AUX_MU_MCR: ReadWrite<u32, AUX_MU_MCR::Register>),
        (0x54 => pub AUX_MU_LSR: ReadOnly<u32, AUX_MU_LSR::Register>),
        (0x58 => _reserved3),
        (0x60 => pub AUX_MU_CNTL: ReadWrite<u32, AUX_MU_CNTL::Register>),
        (0x64 => _reserved4),
        (0x68 => pub AUX_MU_BAUD: ReadWrite<u32>),
        (0x6c => _reserved5),
        (0xd8 => @END),
    }
}

pub type Registers = MMIODerefWrapper<RegisterBlock>;
