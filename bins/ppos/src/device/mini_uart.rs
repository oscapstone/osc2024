use core::fmt;
use core::fmt::Write;

use alloc::{
    collections::VecDeque,
    rc::{Rc, Weak},
    string::String,
    vec::Vec,
};
use tock_registers::{
    interfaces::{ReadWriteable, Readable, Writeable},
    register_bitfields, register_structs,
    registers::{ReadOnly, ReadWrite},
};
use vfs::file::{Umode, UMODE};

use crate::file_system::{
    directory_cache::{DirectoryEntry, DirectoryEntryOperation},
    file::{File, FileOperation},
    inode::{INode, INodeOperation},
    super_block::{SuperBlock, SuperBlockOperation},
    virtual_file_system,
};

use super::{
    common::MMIODerefWrapper,
    interrupt_controller::InterruptNumber,
    interrupt_manager::{self, InterruptHandler, InterruptHandlerDescriptor, InterruptPrehook},
};

use super::device_driver::DeviceDriver;
use library::{
    collections::ring_buffer::RingBuffer,
    console::{self, console, ConsoleMode},
    println,
    sync::mutex::Mutex,
    time::Time,
};

struct MiniUartInner {
    registers: MMIODerefWrapper<Registers>,
    read_buffer: VecDeque<u8>,
    write_buffer: VecDeque<u8>,
    mode: ConsoleMode,
}

pub struct MiniUart {
    inner: Mutex<MiniUartInner>,
    super_block: Mutex<Option<Rc<MiniUartSuperBlock>>>,
}

register_bitfields![
    u32,
    AUX_ENB  [
        SPI2_ENABLE OFFSET(2) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ],
        SPI1_ENABLE OFFSET(1) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ],
        MINI_UART_ENABLE OFFSET(0) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ]
    ],
    AUX_MU_IER [
        ENABLE_TRANSMIT_INTERRUPT OFFSET(1) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ],
        ENABLE_RECEIVE_INTERRUPT OFFSET(0) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ],
    ],
    AUX_MU_IIR [
        INTERRUPT_ID_BITS OFFSET(1) NUMBITS(2) [
            NO_INTERRUPT = 0b00,
            TRANSMIT_HOLDING_REGISTER_EMPTY = 0b01,
            RECEIVER_HOLDS_VAILD_BYTE = 0b10,
        ],
        FIFO_CLEAR_BITS OFFSET(1) NUMBITS(2) [
            CLEAR_RECEIVE_FIFO = 0b01,
            CLEAR_TRANSMIT_FIFO = 0b10,
        ],
        INTERRUPT_PENDING OFFSET(0) NUMBITS(1) [
            PENDING = 0,
        ],
    ],
    AUX_MU_LCR [
        DATA_SIZE OFFSET(0) NUMBITS(1) [
            SEVEN_BITS = 0,
            EIGHT_BITS = 1,
        ]
    ],
    AUX_MU_MCR [
        RTS OFFSET(1) NUMBITS(1) [
            HIGH = 0,
            LOW = 1,
        ]
    ],
    AUX_MU_LSR [
        TRANSMITTER_IDLE OFFSET(6) NUMBITS(1) [
            IDLE = 1,
        ],
        TRANSMITTER_EMPTY OFFSET(5) NUMBITS(1) [
            EMPTY = 1,
        ],
        RECEIVER_OVERRUN OFFSET(1) NUMBITS(1) [
            OVERRUN = 1,
        ],
        DATA_READY OFFSET(0) NUMBITS(1) [
            READY = 1,
        ]
    ],
    AUX_MU_CNTL [
        TRANSMIT_AUTO_FLOW_CONTROL OFFSET(3) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1
        ],
        RECEIVE_AUTO_FLOW_CONTROL OFFSET(2) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ],
        TRANSMITTER_ENABLE OFFSET(1) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ],
        RECEIVER_ENABLE OFFSET(0) NUMBITS(1) [
            DISABLE = 0,
            ENABLE = 1,
        ]
    ]
];

register_structs! {
    Registers {
        (0x00 => _reserved1),
        (0x04 => enable: ReadWrite<u32, AUX_ENB::Register>),
        (0x08 => _reserved2),
        (0x40 => data: ReadWrite<u32>),
        (0x44 => interrupt_enable: ReadWrite<u32, AUX_MU_IER::Register>),
        (0x48 => interrupt_identify: ReadWrite<u32, AUX_MU_IIR::Register>),
        (0x4c => line_controll: ReadWrite<u32, AUX_MU_LCR::Register>),
        (0x50 => modem_controll: ReadWrite<u32, AUX_MU_MCR::Register>),
        (0x54 => line_status: ReadOnly<u32, AUX_MU_LSR::Register>),
        (0x58 => _reserved3),
        (0x60 => controll: ReadWrite<u32, AUX_MU_CNTL::Register>),
        (0x64 => _reserved4),
        (0x68 => baudrate: ReadWrite<u32>),
        (0x6c => _reserved5),
        (0xd8 => @END),
    }
}

impl MiniUartInner {
    const BUFFER_SIZE: usize = 1024;

    /**
     * # Safety
     *
     * - The user must ensure to provide a correct MMIO start address.
     */
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: MMIODerefWrapper::new(mmio_start_addr),
            read_buffer: VecDeque::new(),
            write_buffer: VecDeque::new(),
            mode: ConsoleMode::Sync,
        }
    }

    fn init(&self) {
        self.registers
            .enable
            .modify(AUX_ENB::MINI_UART_ENABLE::ENABLE);
        // disable transmitter and receiver during configuration
        self.registers.controll.modify(
            AUX_MU_CNTL::TRANSMITTER_ENABLE::DISABLE + AUX_MU_CNTL::RECEIVER_ENABLE::DISABLE,
        );
        // disable interrupt which is not needed currently
        self.registers.interrupt_enable.modify(
            AUX_MU_IER::ENABLE_TRANSMIT_INTERRUPT::DISABLE
                + AUX_MU_IER::ENABLE_RECEIVE_INTERRUPT::DISABLE,
        );
        // set the data size to 8 bit
        self.registers
            .line_controll
            .modify(AUX_MU_LCR::DATA_SIZE::EIGHT_BITS);
        // disable auto flow control
        self.registers.modem_controll.set(0);
        // set baud rate to 115200
        self.registers.baudrate.set(270);
        // disable FIFO
        self.registers.interrupt_identify.modify(
            AUX_MU_IIR::FIFO_CLEAR_BITS::CLEAR_TRANSMIT_FIFO
                + AUX_MU_IIR::FIFO_CLEAR_BITS::CLEAR_RECEIVE_FIFO,
        );
        // enable transmitter and receiver
        self.registers
            .controll
            .modify(AUX_MU_CNTL::TRANSMITTER_ENABLE::ENABLE + AUX_MU_CNTL::RECEIVER_ENABLE::ENABLE);
    }

    /**
     * Check if data is available to read
     */
    #[inline(always)]
    fn is_readable(&self) -> bool {
        self.registers.line_status.is_set(AUX_MU_LSR::DATA_READY)
    }

    /**
     * Check if data is a available to write
     */
    #[inline(always)]
    fn is_writable(&self) -> bool {
        self.registers
            .line_status
            .is_set(AUX_MU_LSR::TRANSMITTER_EMPTY)
    }

    fn read_byte(&mut self) -> u8 {
        while !self.is_readable() {
            core::hint::spin_loop();
        }
        self.registers.data.get() as u8
    }

    fn write_byte(&mut self, value: u8) {
        while !self.is_writable() {
            core::hint::spin_loop();
        }
        self.registers.data.set(value as u32);
    }

    fn read_byte_async(&mut self) -> Option<u8> {
        // critical section
        // read buffer is shared between interrupt handlers
        self.disable_read_interrupt();
        let c = self.read_buffer.pop_front();
        self.enable_read_interrupt();
        c
    }

    fn write_byte_async(&mut self, value: u8) {
        // critical section
        // write buffer is shared between interrupt handlers
        self.disable_write_interrupt();
        self.write_buffer.push_back(value);
        self.enable_write_interrupt();
    }

    #[inline(always)]
    fn enable_read_interrupt(&self) {
        self.registers
            .interrupt_enable
            .modify(AUX_MU_IER::ENABLE_RECEIVE_INTERRUPT::ENABLE);
    }

    #[inline(always)]
    fn enable_write_interrupt(&self) {
        self.registers
            .interrupt_enable
            .modify(AUX_MU_IER::ENABLE_TRANSMIT_INTERRUPT::ENABLE);
    }

    #[inline(always)]
    fn disable_read_interrupt(&self) {
        self.registers
            .interrupt_enable
            .modify(AUX_MU_IER::ENABLE_RECEIVE_INTERRUPT::DISABLE);
    }

    #[inline(always)]
    fn disable_write_interrupt(&self) {
        self.registers
            .interrupt_enable
            .modify(AUX_MU_IER::ENABLE_TRANSMIT_INTERRUPT::DISABLE);
    }

    fn handle_interrupt(&mut self) {
        match self
            .registers
            .interrupt_identify
            .read_as_enum(AUX_MU_IIR::INTERRUPT_ID_BITS)
        {
            Some(AUX_MU_IIR::INTERRUPT_ID_BITS::Value::NO_INTERRUPT) => (),
            Some(AUX_MU_IIR::INTERRUPT_ID_BITS::Value::TRANSMIT_HOLDING_REGISTER_EMPTY) => {
                // if nothing to write, disable write interrupt
                // or it will keep firing and racing cpu
                self.disable_write_interrupt();
                if let Some(byte) = self.write_buffer.pop_front() {
                    self.write_byte(byte);
                    self.enable_write_interrupt();
                }
            }
            Some(AUX_MU_IIR::INTERRUPT_ID_BITS::Value::RECEIVER_HOLDS_VAILD_BYTE) => {
                self.disable_read_interrupt();
                let byte = self.read_byte();
                self.read_buffer.push_back(byte);
                self.enable_read_interrupt();
            }
            None => panic!("Invalid interrupt"),
        }
    }

    fn interrupt_prehook(&mut self) {
        match self
            .registers
            .interrupt_identify
            .read_as_enum(AUX_MU_IIR::INTERRUPT_ID_BITS)
        {
            Some(AUX_MU_IIR::INTERRUPT_ID_BITS::Value::NO_INTERRUPT) => (),
            Some(AUX_MU_IIR::INTERRUPT_ID_BITS::Value::TRANSMIT_HOLDING_REGISTER_EMPTY) => {
                self.disable_write_interrupt();
            }
            Some(AUX_MU_IIR::INTERRUPT_ID_BITS::Value::RECEIVER_HOLDS_VAILD_BYTE) => {
                self.disable_read_interrupt();
            }
            None => panic!("Invalid interrupt"),
        }
    }
}

impl MiniUart {
    /**
     * # Safety
     *
     * - The user must ensure to provide a correct MMIO start address.
     */
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: Mutex::new(MiniUartInner::new(mmio_start_addr)),
            super_block: Mutex::new(None),
        }
    }
}

impl fmt::Write for MiniUartInner {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        match self.mode {
            ConsoleMode::Sync => {
                for c in s.chars() {
                    self.write_byte(c as u8);
                }
            }
            ConsoleMode::Async => {
                for c in s.chars() {
                    self.write_byte_async(c as u8);
                }
            }
        }
        Ok(())
    }
}

impl console::Write for MiniUart {
    fn write_char(&self, c: char) {
        let mut inner = self.inner.lock().unwrap();
        match inner.mode {
            ConsoleMode::Sync => inner.write_byte(c as u8),
            ConsoleMode::Async => inner.write_byte_async(c as u8),
        }
    }

    fn write_fmt(&self, args: fmt::Arguments) -> fmt::Result {
        let mut inner = self.inner.lock().unwrap();
        inner.write_fmt(args)
    }
}

impl console::Read for MiniUart {
    fn read_char(&self) -> Option<char> {
        let mut inner = self.inner.lock().unwrap();
        match inner.mode {
            ConsoleMode::Sync => Some(inner.read_byte() as char),
            ConsoleMode::Async => inner.read_byte_async().map(|byte| byte as char),
        }
    }
}

impl console::ReadWrite for MiniUart {}

impl console::Console for MiniUart {
    fn change_mode(&self, mode: ConsoleMode) {
        let mut inner = self.inner.lock().unwrap();
        inner.read_buffer.clear();
        // inner.write_buffer.clear();
        inner.mode = mode;
    }
}

impl InterruptPrehook for MiniUart {
    fn prehook(&self) -> Result<(), &'static str> {
        self.inner.lock().unwrap().interrupt_prehook();
        Ok(())
    }
}

impl InterruptHandler for MiniUart {
    fn handle(&self) -> Result<(), &'static str> {
        self.inner.lock().unwrap().handle_interrupt();
        Ok(())
    }
}

#[derive(Debug)]
pub struct MiniUartSuperBlock {
    inodes: Mutex<Vec<Rc<dyn INodeOperation>>>,
}

impl MiniUartSuperBlock {
    pub fn new() -> Self {
        Self {
            inodes: Mutex::new(Vec::with_capacity(1)),
        }
    }
}

impl SuperBlockOperation for MiniUartSuperBlock {
    fn add_inode(&self, inode: Rc<dyn INodeOperation>) {
        self.inodes.lock().unwrap().push(inode);
    }

    fn set_root(&self, root: Weak<dyn DirectoryEntryOperation>) {
        unimplemented!()
    }

    fn root(&self) -> Option<Weak<dyn DirectoryEntryOperation>> {
        unimplemented!()
    }
}

#[derive(Debug)]
pub struct MiniUartFile {
    inner: File,
}

impl MiniUartFile {
    pub fn new(inode: Rc<dyn INodeOperation>) -> Self {
        Self {
            inner: File::new(inode),
        }
    }
}

impl FileOperation for MiniUartFile {
    fn write(&self, buf: &[u8], len: usize) -> Result<usize, &'static str> {
        console().write_str(match core::str::from_utf8(buf) {
            Ok(s) => s,
            Err(_) => return Err("Invalid UTF-8 sequence"),
        });
        Ok(len)
    }

    fn read(&self, buf: &mut [u8], len: usize) -> Result<usize, &'static str> {
        let mut count = 0;
        console().change_mode(ConsoleMode::Sync);
        while let Some(c) = console().read_char() {
            if count >= len {
                break;
            }
            buf[count] = c as u8;
            count += 1;
        }
        console().change_mode(ConsoleMode::Async);
        Ok(count)
    }

    fn close(&self) {
        self.inner.close()
    }

    fn position(&self) -> usize {
        0
    }

    fn set_position(&self, position: usize) {
        unimplemented!()
    }

    fn inode(&self) -> Rc<dyn INodeOperation> {
        self.inner.inode()
    }
}

#[derive(Debug)]
pub struct MiniUartINode {
    inner: INode,
}

impl MiniUartINode {
    pub fn new(super_block: Weak<MiniUartSuperBlock>) -> Self {
        Self {
            inner: INode::new(
                Umode::new(UMODE::OWNER_READ::SET),
                0,
                0,
                Time::new(0, 0),
                Time::new(0, 0),
                Time::new(0, 0),
                0,
                super_block,
            ),
        }
    }
}

impl INodeOperation for MiniUartINode {
    fn lookup(
        &self,
        parent_directory: Rc<dyn DirectoryEntryOperation>,
        target_name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>> {
        self.inner.lookup(parent_directory, target_name)
    }

    fn create(
        &self,
        umode: Umode,
        name: alloc::string::String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        unimplemented!()
    }

    fn mkdir(
        &self,
        umode: Umode,
        name: alloc::string::String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        unimplemented!()
    }

    fn open(&self, inode: Rc<dyn INodeOperation>) -> Rc<dyn FileOperation> {
        Rc::new(MiniUartFile::new(inode))
    }

    fn size(&self) -> usize {
        0
    }

    fn super_block(&self) -> Weak<dyn SuperBlockOperation> {
        self.inner.super_block()
    }
}

impl DeviceDriver for MiniUart {
    type InterruptNumberType = InterruptNumber;

    unsafe fn init(&self) -> Result<(), &'static str> {
        let inner = self.inner.lock().unwrap();
        inner.init();
        *self.super_block.lock().unwrap() = Some(Rc::new(MiniUartSuperBlock::new()));
        let dev_folder = virtual_file_system()
            .find_directory_entry(&Some(virtual_file_system().root().unwrap().root), "dev")
            .unwrap();
        let super_block = self.super_block.lock().unwrap().as_ref().unwrap().clone();
        let mini_uart_inode = Rc::new(MiniUartINode::new(Rc::downgrade(&super_block)));
        super_block.add_inode(mini_uart_inode.clone() as Rc<dyn INodeOperation>);
        let mini_uart_dentry = Rc::new(DirectoryEntry::new(
            Some(Rc::downgrade(&dev_folder)),
            String::from("uart"),
            Rc::downgrade(&(mini_uart_inode as Rc<dyn INodeOperation>)),
            Rc::downgrade(&(super_block.clone() as Rc<dyn SuperBlockOperation>)),
        ));
        virtual_file_system().add_directory_entry(mini_uart_dentry.clone());
        dev_folder.add_child(Rc::downgrade(
            &(mini_uart_dentry as Rc<dyn DirectoryEntryOperation>),
        ));
        Ok(())
    }

    fn register_and_enable_interrupt_handler(
        &'static self,
        interrupt_number: &Self::InterruptNumberType,
    ) -> Result<(), &'static str> {
        let descriptor =
            InterruptHandlerDescriptor::new(*interrupt_number, "mini uart", Some(self), self, 0);
        interrupt_manager::interrupt_manager().register_handler(descriptor)?;
        interrupt_manager::interrupt_manager().enable(interrupt_number);
        let inner = self.inner.lock().unwrap();
        inner.enable_read_interrupt();
        inner.enable_write_interrupt();
        Ok(())
    }
}
