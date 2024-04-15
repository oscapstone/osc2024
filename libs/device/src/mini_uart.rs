use core::fmt;
use core::fmt::Write;

use tock_registers::{
    interfaces::{ReadWriteable, Readable, Writeable},
    register_bitfields, register_structs,
    registers::{ReadOnly, ReadWrite},
};

use crate::{
    common::MMIODerefWrapper,
    interrupt_controller::InterruptNumber,
    interrupt_manager::{self, InterruptHandler, InterruptHandlerDescriptor, InterruptPrehook},
};

use super::device_driver::DeviceDriver;
use library::{collections::ring_buffer::RingBuffer, console, sync::mutex::Mutex};

struct MiniUartInner {
    registers: MMIODerefWrapper<Registers>,
    read_buffer: RingBuffer<{ Self::BUFFER_SIZE }>,
    write_buffer: RingBuffer<{ Self::BUFFER_SIZE }>,
}

pub struct MiniUart {
    inner: Mutex<MiniUartInner>,
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
            read_buffer: RingBuffer::new(),
            write_buffer: RingBuffer::new(),
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
        while !self.is_readable() {}
        self.registers.data.get() as u8
    }

    fn write_byte(&mut self, value: u8) {
        while !self.is_writable() {}
        self.registers.data.set(value as u32);
    }

    fn read_byte_async(&mut self) -> Option<u8> {
        // critical section
        // read buffer is shared between interrupt handlers
        self.disable_read_interrupt();
        let c = self.read_buffer.pop();
        self.enable_read_interrupt();
        c
    }

    fn write_byte_async(&mut self, value: u8) {
        // critical section
        // write buffer is shared between interrupt handlers
        self.disable_write_interrupt();
        self.write_buffer.push(value);
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
                if let Some(byte) = self.write_buffer.pop() {
                    self.write_byte(byte);
                } else {
                    // nothing to write, disable write interrupt
                    // or it will keep firing and racing cpu
                    self.disable_write_interrupt();
                }
            }
            Some(AUX_MU_IIR::INTERRUPT_ID_BITS::Value::RECEIVER_HOLDS_VAILD_BYTE) => {
                let byte = self.read_byte();
                self.read_buffer.push(byte);
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
        }
    }
}

impl fmt::Write for MiniUartInner {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for c in s.chars() {
            self.write_byte_async(c as u8);
        }
        Ok(())
    }
}

impl console::Read for MiniUart {
    fn read_char(&self) -> char {
        let mut inner = self.inner.lock().unwrap();
        inner.read_byte() as char
    }
}

impl console::Write for MiniUart {
    fn write_char(&self, c: char) {
        let mut inner = self.inner.lock().unwrap();
        inner.write_byte(c as u8);
    }

    fn write_fmt(&self, args: fmt::Arguments) -> fmt::Result {
        let mut inner = self.inner.lock().unwrap();
        inner.write_fmt(args)
    }
}

impl console::ReadWrite for MiniUart {}

impl console::AsyncRead for MiniUart {
    fn read_char_async(&self) -> Option<char> {
        let mut inner = self.inner.lock().unwrap();
        inner.read_byte_async().map(|byte| byte as char)
    }
}

impl console::AsyncWrite for MiniUart {
    fn write_char_async(&self, c: char) {
        let mut inner = self.inner.lock().unwrap();
        // inner.write_byte(c as u8);
        inner.write_byte_async(c as u8);
    }

    fn write_fmt_async(&self, args: fmt::Arguments) -> fmt::Result {
        let mut inner = self.inner.lock().unwrap();
        inner.write_fmt(args)
    }
}

impl console::AsyncReadWrite for MiniUart {}

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

impl DeviceDriver for MiniUart {
    type InterruptNumberType = InterruptNumber;

    unsafe fn init(&self) -> Result<(), &'static str> {
        let inner = self.inner.lock().unwrap();
        inner.init();
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
