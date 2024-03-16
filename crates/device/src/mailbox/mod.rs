mod macros;
mod registers;

use small_std::{print, sync::Mutex};
use tock_registers::interfaces::{Readable, Writeable};

use crate::driver::DeviceDriver;
use macros::{define_mailbox_message, try_enum_from_repr};
use registers::{Registers, MAILBOX_STATUS};

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
enum MailboxRequestCode {
    ProcessRequest = 0,
}

#[repr(u32)]
enum MailboxResponseCode {
    Success = 0x8000_0000,
    ParseFailed = 0x8000_0001,
}

try_enum_from_repr! {
    from = u32,
    to = MailboxResponseCode,
    error = &'static str,
    variants = {
        MailboxResponseCode::Success,
        MailboxResponseCode::ParseFailed,
    },
    fallback = "invalid code"
}

#[repr(u32)]
enum TagIdentifier {
    GetBoardRevision = 0x0001_0002,
    GetARMMemory = 0x0001_0005,
    End = 0x0000_0000,
}

pub struct ARMMemoryInfo {
    pub base_address: u32,
    pub size: u32,
}

impl MailboxInner {
    fn get_board_revision(&self) -> Result<u32, &'static str> {
        define_mailbox_message! {
            Message,
            board_revision => {
                board_revision: u32
            },
        };

        let mut message = Message {
            buffer_size: 7 * 4,
            code: MailboxRequestCode::ProcessRequest as u32,
            board_revision_identifier: TagIdentifier::GetBoardRevision,
            board_revision_buffer_size: 4,
            board_revision_code: 0,
            board_revision: 0,
            end_tag: TagIdentifier::End,
        };
        // SAFETY: The buffer is properly aligned and has the correct size
        self.call(unsafe { core::mem::transmute(core::ptr::addr_of_mut!(message)) });

        use MailboxResponseCode as MRC;
        match MRC::try_from(message.code) {
            Ok(MRC::Success) => Ok(message.board_revision),
            Ok(MRC::ParseFailed) => Err("Error parsing request buffer"),
            _ => Err("Invalid response received"),
        }
    }

    fn get_arm_memory(&self) -> Result<ARMMemoryInfo, &'static str> {
        define_mailbox_message! {
            Message,
            arm_memory => {
                base_address: u32,
                size: u32
            },
        };

        let mut message = Message {
            buffer_size: 8 * 4,
            code: MailboxRequestCode::ProcessRequest as u32,
            arm_memory_identifier: TagIdentifier::GetARMMemory,
            arm_memory_buffer_size: 8,
            arm_memory_code: 0,
            base_address: 0,
            size: 0,
            end_tag: TagIdentifier::End,
        };
        // SAFETY: The buffer is properly aligned and has the correct size
        self.call(unsafe { core::mem::transmute(core::ptr::addr_of_mut!(message)) });

        use MailboxResponseCode as MRC;
        match MRC::try_from(message.code) {
            Ok(MRC::Success) => {
                let Message {
                    base_address, size, ..
                } = message;
                Ok(ARMMemoryInfo { base_address, size })
            }
            Ok(MRC::ParseFailed) => Err("Error parsing request buffer"),
            _ => Err("Invalid response received"),
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

    pub fn get_board_revision(&self) -> Result<u32, &str> {
        let inner = self.inner.lock().unwrap();
        inner.get_board_revision()
    }

    pub fn get_arm_memory(&self) -> Result<ARMMemoryInfo, &str> {
        let inner = self.inner.lock().unwrap();
        inner.get_arm_memory()
    }
}

impl DeviceDriver for Mailbox {
    fn compatible(&self) -> &'static str {
        Self::COMPATIBLE
    }
}
