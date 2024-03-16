use small_std::{print, sync::Mutex};
use tock_registers::{
    interfaces::{Readable, Writeable},
    register_bitfields, register_structs,
    registers::{ReadOnly, WriteOnly},
};

use crate::{common::MMIODerefWrapper, driver::DeviceDriver};

register_bitfields! {
    u32,

    MAILBOX_STATUS [
        FULL OFFSET(31) NUMBITS(1) [
            NotFull = 0,
            Full = 1,
        ],
        EMPTY OFFSET(30) NUMBITS(1) [
            NotEmpty = 0,
            Empty = 1,
        ]
    ],
    MAILBOX_READ [
        DATA OFFSET(4) NUMBITS(28) [],
        CHANNEL OFFSET(0) NUMBITS(4) []
    ],
    MAILBOX_WRITE [
        DATA OFFSET(4) NUMBITS(28) [],
        CHANNEL OFFSET(0) NUMBITS(4) []
    ]
}

register_structs! {
    #[allow(non_snake_case)]
    RegisterBlock {
        (0x00 => MAILBOX_READ: ReadOnly<u32, MAILBOX_READ::Register>),
        (0x04 => _reserved1),
        (0x18 => MAILBOX_STATUS: ReadOnly<u32, MAILBOX_STATUS::Register>),
        (0x1c => _reserved2),
        (0x20 => MAILBOX_WRITE: WriteOnly<u32, MAILBOX_WRITE::Register>),
        (0x24 => @END),
    }
}

type Registers = MMIODerefWrapper<RegisterBlock>;

struct MailboxInner {
    registers: Registers,
}

pub struct Mailbox {
    inner: Mutex<MailboxInner>,
}

impl MailboxInner {
    const CHANNEL_MASK: u32 = 0b1111;

    /// SAFETY: The user msut ensure to provide a correct MMIO start address
    const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: Registers::new(mmio_start_addr),
        }
    }

    fn is_writable(&self) -> bool {
        !self.registers.MAILBOX_STATUS.is_set(MAILBOX_STATUS::FULL)
    }

    fn is_readable(&self) -> bool {
        !self.registers.MAILBOX_STATUS.is_set(MAILBOX_STATUS::EMPTY)
    }

    fn read(&self, channel: u8) -> *mut u32 {
        loop {
            while !self.is_readable() {}
            let message = self.registers.MAILBOX_READ.get();
            let data = message & !Self::CHANNEL_MASK;
            let data_channel = (message & Self::CHANNEL_MASK) as u8;
            if data_channel == channel {
                return data as *mut u32;
            }
        }
    }

    fn write(&self, channel: u8, buffer_addr: *mut u32) {
        while !self.is_writable() {}
        let message_addr = buffer_addr as u32 & !Self::CHANNEL_MASK;
        print!(""); // WTF??
        self.registers
            .MAILBOX_WRITE
            .set(message_addr | channel as u32);
    }

    fn call(&self, buffer_addr: *mut u32) -> *mut u32 {
        self.write(8, buffer_addr);
        self.read(8)
    }
}

#[repr(u32)]
enum BufferRequestCode {
    ProcessRequest = 0,
}

#[repr(u32)]
enum TagIdentifier {
    GetBoardRevision = 0x00010002,
    GetARMMemory = 0x00010005,
}

pub struct ARMMemoryInfo {
    pub base_address: u32,
    pub size: u32,
}

impl MailboxInner {
    fn get_board_revision(&self) -> u32 {
        #[repr(align(16))]
        struct Message {
            data: [u32; 7],
        }

        let mut message = Message {
            data: [
                /* length of message in bytes */ 7 * 4,
                /* code */ BufferRequestCode::ProcessRequest as u32,
                /* tag identifier */ TagIdentifier::GetBoardRevision as u32,
                /* length of result in bytes */ 4,
                /* request code */ 0,
                /* board revision */ 0,
                /* end tag */ 0,
            ],
        };
        self.call(message.data.as_mut_ptr());
        message.data[5]
    }

    fn get_arm_memory(&self) -> ARMMemoryInfo {
        #[repr(align(16))]
        struct Message {
            data: [u32; 8],
        }

        let mut message = Message {
            data: [
                /* length of message in bytes */ 8 * 4,
                /* code */ BufferRequestCode::ProcessRequest as u32,
                /* tag identifier */ TagIdentifier::GetARMMemory as u32,
                /* length of result in bytes */ 8,
                /* request code */ 0,
                /* base address */ 0,
                /* size */ 0,
                /* end tag */ 0,
            ],
        };
        self.call(message.data.as_mut_ptr());

        ARMMemoryInfo {
            base_address: message.data[5],
            size: message.data[6],
        }
    }
}

impl Mailbox {
    const COMPATIBLE: &'static str = "Mailbox";

    /// SAFETY: The user msut ensure to provide a correct MMIO start address
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: Mutex::new(MailboxInner::new(mmio_start_addr)),
        }
    }

    pub fn get_board_revision(&self) -> u32 {
        let inner = self.inner.lock().unwrap();
        inner.get_board_revision()
    }

    pub fn get_arm_memory(&self) -> ARMMemoryInfo {
        let inner = self.inner.lock().unwrap();
        inner.get_arm_memory()
    }
}

impl DeviceDriver for Mailbox {
    fn compatible(&self) -> &'static str {
        Self::COMPATIBLE
    }
}
