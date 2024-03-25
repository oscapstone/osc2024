use crate::synchronization::interface::Mutex;
use crate::{bcm::common::MMIODerefWrapper, synchronization::NullLock};
use aarch64_cpu::asm::nop;
use core::usize;
use tock_registers::{
    interfaces::{Readable, Writeable},
    register_bitfields, register_structs,
    registers::ReadWrite,
};

const REQUEST_CODE: u32 = 0x0000_0000;
const TAG_REQUEST_CODE: u32 = 0x0000_0000;
const END_TAG: u32 = 0x0000_0000;

#[derive(Copy, Clone)]
pub enum MailboxTag {
    BoardRevision = 0x0001_0002,
    BoardSerial = 0x0001_10004,
    ArmMemory = 0x0001_0005,
    VcMemory = 0x0001_0006,
}

register_bitfields! {
    u32,

    MBOX_READ [
        DATA OFFSET(0) NUMBITS(32) []
    ],
    MBOX_POLL [
        DATA OFFSET(0) NUMBITS(32) []
    ],
    MBOX_SENDER [
        DATA OFFSET(0) NUMBITS(32) []
    ],
    MBOX_STATUS [
        DATA OFFSET(0) NUMBITS(32) [
            FULL = 0x8000_0000,
            EMPTY = 0x4000_0000
        ],
    ],
    MBOX_CONFIG [
        DATA OFFSET(0) NUMBITS(32) []
    ],
    MBOX_WRITE [
        DATA OFFSET(0) NUMBITS(32) []
    ]
}

register_structs! {
    #[allow(non_snake_case)]
    RegisterBlock {
        (0x00 => MBOC_READ: ReadWrite<u32, MBOX_READ::Register>),
        (0x04 => _reserved1),
        (0x10 => MBOC_POLL: ReadWrite<u32, MBOX_POLL::Register>),
        (0x14 => MBOC_SENDER: ReadWrite<u32, MBOX_SENDER::Register>),
        (0x18 => MBOC_STATUS: ReadWrite<u32, MBOX_STATUS::Register>),
        (0x1C => MBOC_CONFIG: ReadWrite<u32, MBOX_CONFIG::Register>),
        (0x20 => MBOC_WRITE: ReadWrite<u32, MBOX_WRITE::Register>),
        (0x24 => @END),
    }
}

type Registers = MMIODerefWrapper<RegisterBlock>;

struct MailboxInner {
    registers: Registers,
}

#[repr(C, align(16))]
struct MailboxMsg {
    buffer: [u32; 8],
}

impl MailboxInner {
    const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: MMIODerefWrapper::new(mmio_start_addr),
        }
    }

    fn call(&mut self, mailbox: &mut [u32]) {
        let mut mailbox_addr = mailbox.as_ptr() as u32;
        // last 4 bits must be 8(channel number 8)
        mailbox_addr = mailbox_addr & !0xF | 8;

        // wait until mailbox is not full
        while self
            .registers
            .MBOC_STATUS
            .matches_all(MBOX_STATUS::DATA::FULL)
        {
            nop();
        }

        // write the address of the mailbox
        self.registers
            .MBOC_WRITE
            .write(MBOX_WRITE::DATA.val(mailbox_addr));

        // wait until mailbox is get our address
        loop {
            while self
                .registers
                .MBOC_STATUS
                .matches_all(MBOX_STATUS::DATA::EMPTY)
            {
                nop();
            }

            if self.registers.MBOC_READ.read(MBOX_READ::DATA) == mailbox_addr {
                break;
            }
        }
    }

    pub fn get_msg(&mut self, tag: MailboxTag) -> (u32, u32) {
        let mut mailbox = MailboxMsg { buffer: [0; 8] };
        match tag {
            MailboxTag::BoardRevision => {
                mailbox.buffer[0] = 7 * 4; // buffer size in bytes
                mailbox.buffer[3] = 4; // maximum of request and response value buffer's length.
                mailbox.buffer[6] = END_TAG;
            }
            MailboxTag::BoardSerial => {
                mailbox.buffer[0] = 8 * 4; // buffer size in bytes
                mailbox.buffer[3] = 8; // maximum of request and response value buffer's length.
                mailbox.buffer[7] = END_TAG;
            }
            MailboxTag::ArmMemory => {
                mailbox.buffer[0] = 8 * 4; // buffer size in bytes
                mailbox.buffer[3] = 8; // maximum of request and response value buffer's length.
                mailbox.buffer[7] = END_TAG;
            }
            MailboxTag::VcMemory => {
                mailbox.buffer[0] = 8 * 4; // buffer size in bytes
                mailbox.buffer[3] = 8; // maximum of request and response value buffer's length.
                mailbox.buffer[7] = END_TAG;
            }
        }

        mailbox.buffer[1] = REQUEST_CODE;
        mailbox.buffer[2] = tag as u32; // tag identifier
        mailbox.buffer[4] = TAG_REQUEST_CODE;
        mailbox.buffer[5] = 0; // value buffer
        mailbox.buffer[6] = 0; // value buffer

        self.call(&mut mailbox.buffer);

        match tag {
            MailboxTag::BoardRevision => (mailbox.buffer[5], 0),
            MailboxTag::BoardSerial => (mailbox.buffer[5], mailbox.buffer[6]),
            MailboxTag::ArmMemory => (mailbox.buffer[5], mailbox.buffer[6]),
            MailboxTag::VcMemory => (mailbox.buffer[5], mailbox.buffer[6]),
        }
    }
}

pub struct Mailbox {
    inner: NullLock<MailboxInner>,
}

impl Mailbox {
    /// Create an instance.
    ///
    /// # Safety
    ///
    /// - The caller must ensure that the `base_addr` is valid.
    pub const unsafe fn new(base_addr: usize) -> Self {
        Self {
            inner: NullLock::new(MailboxInner::new(base_addr)),
        }
    }

    pub fn get(&self, tag: MailboxTag) -> (u32, u32) {
        self.inner.lock(|inner| inner.get_msg(tag))
    }
}
