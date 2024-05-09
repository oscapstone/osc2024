// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

//! MBOX Driver.

use crate::println;

use crate::{
    bsp::device_driver::common::MMIODerefWrapper,
    driver, mbox,
    synchronization::{interface::Mutex, NullLock},
};

use aarch64_cpu::asm;

use tock_registers::{
    interfaces::{Readable, Writeable},
    register_bitfields, register_structs,
    registers::ReadWrite,
};

//--------------------------------------------------------------------------------------------------
// Private Definitions
//--------------------------------------------------------------------------------------------------

// MBOX registers

const CHANNEL_GPU: u32 = 8;

register_bitfields! {
  u32,

  MBOX_READ_REG [
    DATA OFFSET(0) NUMBITS(32) []
  ],
  MBOX_PEEK_REG [
    DATA OFFSET(0) NUMBITS(32) []
  ],
  MBOX_SENDER_REG [
    DATA OFFSET(0) NUMBITS(32) []
  ],
  MBOX_STATUS_REG [
    DATA OFFSET(0) NUMBITS(32) [
      empty = 0x4000_0000,
      full = 0x8000_0000
    ]
  ],
  MBOX_CONFIG_REG [
    DATA OFFSET(0) NUMBITS(32) []
  ],
  MBOX_WRITE_REG [
    DATA OFFSET(0) NUMBITS(32) []
  ]
}

register_structs! {
  #[allow(non_snake_case)]
  RegisterBlock {
      (0x00 => MBOX_READ_REG: ReadWrite<u32, MBOX_READ_REG::Register>),
      (0x04 => _reserved1),
      (0x10 => MBOX_PEEK_REG: ReadWrite<u32, MBOX_PEEK_REG::Register>),
      (0x14 => MBOX_SENDER_REG: ReadWrite<u32, MBOX_SENDER_REG::Register>),
      (0x18 => MBOX_STATUS_REG: ReadWrite<u32, MBOX_STATUS_REG::Register>),
      (0x1C => MBOX_CONFIG_REG: ReadWrite<u32, MBOX_CONFIG_REG::Register>),
      (0x20 => MBOX_WRITE_REG: ReadWrite<u32, MBOX_WRITE_REG::Register>),
      (0x24 => @END),
  }
}

/// Abstraction for the associated MMIO registers.
type Registers = MMIODerefWrapper<RegisterBlock>;

struct MBOXInner {
    registers: Registers,
}

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

/// Representation of the MBOX HW.
pub struct MBOX {
    inner: NullLock<MBOXInner>,
}

//--------------------------------------------------------------------------------------------------
// Private Code
//--------------------------------------------------------------------------------------------------

#[repr(align(64))]
#[derive(Copy, Clone)]
struct MailboxMsg {
    buffer: [u32; 8],
}

impl MBOXInner {
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: Registers::new(mmio_start_addr),
        }
    }

    pub fn init(&mut self) {}

    fn mailbox_read(&mut self, channel: u32) -> u32 {
        loop {
            while self
                .registers
                .MBOX_STATUS_REG
                .matches_all(MBOX_STATUS_REG::DATA::empty)
            {
                asm::nop();
            }

            let data = self.registers.MBOX_READ_REG.get();

            if (data & 0xF) == channel {
                return data;
            }
        }
    }

    fn mailbox_write(&mut self, channel: u32, data: u32) {
        while self
            .registers
            .MBOX_STATUS_REG
            .matches_all(MBOX_STATUS_REG::DATA::full)
        {
            asm::nop();
        }
        self.registers.MBOX_WRITE_REG.set(data | channel);
    }

    fn mailbox_call(&mut self, mailbox: &mut [u32]) -> bool {
        let mailbox_ptr = mailbox.as_mut_ptr() as u32;
        if mailbox_ptr & 0xF != 0 {
            println!("{:x}", mailbox_ptr);
            println!("ptr failed");
            return false;
        }

        Self::mailbox_write(self, CHANNEL_GPU, mailbox_ptr);

        if Self::mailbox_read(self, CHANNEL_GPU) != mailbox_ptr | CHANNEL_GPU {
            println!("Call failed");
            return false;
        }

        true
    }

    pub fn get_board_revision(&mut self) -> u32 {
        let mut mailbox = MailboxMsg { buffer: [0; 8] };
        mailbox.buffer[0] = 7 * 4;
        mailbox.buffer[1] = 0;
        mailbox.buffer[2] = 0x0001_0002;
        mailbox.buffer[3] = 4;
        mailbox.buffer[4] = 0;
        mailbox.buffer[5] = 0;
        mailbox.buffer[6] = 0x0000_0000;
        // let mut mbox = [
        //     7 * 4,       // Buffer size in bytes
        //     0,           // Request/response code
        //     0x0001_0002, // Tag: Get board revision
        //     4,           // Buffer size in bytes
        //     0,           // Tag request code
        //     0,           // Value buffer
        //     0x0000_0000, // End tag
        // ];
        if !Self::mailbox_call(self, &mut mailbox.buffer) {
            println!("get board failed");
            return 0;
        }
        mailbox.buffer[5]
    }

    pub fn get_arm_memory(&mut self) -> (u32, u32) {
        let mut mailbox = MailboxMsg { buffer: [0; 8] };
        mailbox.buffer[0] = 8 * 4;
        mailbox.buffer[1] = 0;
        mailbox.buffer[2] = 0x0001_0005;
        mailbox.buffer[3] = 8;
        mailbox.buffer[4] = 0;
        mailbox.buffer[5] = 0;
        mailbox.buffer[6] = 0;
        mailbox.buffer[7] = 0x0000_0000;
        // let mut mbox = [
        //     8 * 4,       // Buffer size in bytes
        //     0,           // Request/response code
        //     0x0001_0005, // Tag: Get ARM memory
        //     8,           // Buffer size in bytes
        //     0,           // Tag request code
        //     0,           // Value buffer
        //     0,           // Value buffer
        //     0x0000_0000, // End tag
        // ];
        if !Self::mailbox_call(self, &mut mailbox.buffer) {
            println!("get mem failed");
            return (0, 0);
        }
        (mailbox.buffer[5], mailbox.buffer[6])
    }
}

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

impl MBOX {
    pub const COMPATIBLE: &'static str = "RPI3 MBOX";

    /// Create an instance.
    ///
    /// # Safety
    ///
    /// - The user must ensure to provide a correct MMIO start address.
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: NullLock::new(MBOXInner::new(mmio_start_addr)),
        }
    }
}

//------------------------------------------------------------------------------
// OS Interface Code
//------------------------------------------------------------------------------

impl driver::interface::DeviceDriver for MBOX {
    fn compatible(&self) -> &'static str {
        Self::COMPATIBLE
    }

    unsafe fn init(&self) -> Result<(), &'static str> {
        self.inner.lock(|inner| inner.init());
        Ok(())
    }
}

impl mbox::interface::Boardinfo for MBOX {
    /// Passthrough of `args` to the `core::fmt::Write` implementation, but guarded by a Mutex to
    /// serialize access.
    fn get_arm_memory(&self) -> (u32, u32) {
        self.inner.lock(|inner| inner.get_arm_memory())
    }

    fn get_board_revision(&self) -> u32 {
        self.inner.lock(|inner| inner.get_board_revision())
    }
}

impl mbox::interface::All for MBOX {}
