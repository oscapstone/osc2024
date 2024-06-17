use core::arch::asm;
use core::u32;
use core::{
    ptr::{read_volatile, write_volatile},
    usize,
};

pub const AUXENB: u32 = 0x3F215004;
pub const AUX_MU_CNTL_REG: u32 = 0x3F215060;
pub const AUX_MU_IER_REG: u32 = 0x3F215044;
pub const AUX_MU_LCR_REG: u32 = 0x3F21504C;
pub const AUX_MU_MCR_REG: u32 = 0x3F215050;
pub const AUX_MU_BAUD_REG: u32 = 0x3F215068;
pub const AUX_MU_IIR_REG: u32 = 0x3F215048;
pub const AUX_MU_IO_REG: u32 = 0x3F215040;
pub const AUX_MU_LSR_REG: u32 = 0x3F215054;

const GPFSEL_BASE: u32 = 0x3F200000;
const GPFSEL0: u32 = 0x3F200000;
const GPFSEL1: u32 = 0x3F200004;

const GPPUD: u32 = 0x3F200094;
const GPPUDCLK0: u32 = 0x3F200098;
const GPPUDCLK1: u32 = 0x3F20009C;

pub fn is_buf_empty() -> bool {
    unsafe { READ_RING_BUFFER.is_empty() }
}

struct RingBuffer {
    buffer: [u8; 1024],
    head: usize,
    tail: usize,
}

impl RingBuffer {
    pub fn new() -> RingBuffer {
        RingBuffer {
            buffer: [0; 1024],
            head: 0,
            tail: 0,
        }
    }

    pub fn write(&mut self, c: u8) {
        self.buffer[self.head] = c;
        self.head = (self.head + 1) % 1024;
    }

    pub fn read(&mut self) -> Option<u8> {
        if self.head == self.tail {
            None
        } else {
            let c = self.buffer[self.tail];
            self.tail = (self.tail + 1) % 1024;
            Some(c)
        }
    }

    pub fn is_empty(&self) -> bool {
        self.head == self.tail
    }

    pub fn is_full(&self) -> bool {
        (self.head + 1) % 1024 == self.tail
    }
}

static mut READ_RING_BUFFER: RingBuffer = RingBuffer {
    buffer: [0; 1024],
    head: 0,
    tail: 0,
};

static mut WRITE_RING_BUFFER: RingBuffer = RingBuffer {
    buffer: [0; 1024],
    head: 0,
    tail: 0,
};

#[no_mangle]
#[inline(never)]
pub fn push_read_buf(c: u8) {
    unsafe {
        READ_RING_BUFFER.write(c);
    }
}

#[no_mangle]
#[inline(never)]
pub fn pop_read_buf() -> Option<u8> {
    unsafe { READ_RING_BUFFER.read() }
}

#[no_mangle]
#[inline(never)]
pub fn pop_write_buf() -> Option<u8> {
    unsafe { WRITE_RING_BUFFER.read() }
}

#[no_mangle]
#[inline(never)]
pub fn push_write_buf(c: u8) {
    unsafe {
        WRITE_RING_BUFFER.write(c);
    }
}

#[no_mangle]
#[inline(never)]
pub fn async_getline(s: &mut [u8; 128], is_echo: bool) -> &str {
    let mut ptr: usize = 0;
    loop {
        if let Some(i) = pop_read_buf() {
            if is_echo {
                write_u8_buf(i as u8);
                flush();
            }
            if i == 13 {
                write_u8_buf(10);
                break;
            }
            s[ptr] = i as u8;
            ptr = ptr + 1;
        }
    }
    core::str::from_utf8(&s[0..ptr]).unwrap()
}

pub fn flush() {
    write_int(true);
}

pub fn write_u8_buf(c: u8) {
    unsafe {
        WRITE_RING_BUFFER.write(c);
    }
}

#[no_mangle]
#[inline(never)]
pub fn write_int(enable: bool) {
    unsafe {
        // enable interrupt
        if enable {
            write_volatile(AUX_MU_IER_REG as *mut u32, 0b11);
        } else {
            write_volatile(AUX_MU_IER_REG as *mut u32, 0b01);
        }
    }
}

#[no_mangle]
#[inline(never)]
pub fn send_write_buf() {
    loop {
        if let Some(s) = pop_write_buf() {
            unsafe { write_u8(s) };
        } else {
            break;
        }
    }
    write_int(false);
}
// Initialize the UART
#[no_mangle]
#[inline(never)]
pub fn init_uart(int: bool) {
    unsafe {
        // configure GPFSEL1 register to set FSEL14 FSEL15 to ALT5
        let fsel = read_volatile(GPFSEL1 as *mut u32);
        let fsel_mask = !(0b111111 << 12);
        let fsel_set = 0b010010 << 12;
        write_volatile(GPFSEL1 as *mut u32, (fsel & fsel_mask) | fsel_set);

        // configure pull up/down register to disable GPIO pull up/down
        let pud = 0b0;
        write_volatile(GPPUD as *mut u32, pud);

        // wait 150 cycles
        uart_nops();

        // configure pull up/down clock register to disable GPIO pull up/down
        let pudclk0 = !(0b11 << 14);
        write_volatile(GPPUDCLK0 as *mut u32, pudclk0);
        let pudclk1 = 0;
        write_volatile(GPPUDCLK1 as *mut u32, pudclk1);
        // wait 150 cycles
        uart_nops();

        // Write to GPPUD to remove the control signal
        write_volatile(GPPUD as *mut u32, 0);
        // Write to GPPUDCLK0 to remove the clock
        write_volatile(GPPUDCLK0 as *mut u32, 0);

        // write some word to uart to initialize it
        // Set AUXENB register to enable mini UART
        write_volatile(AUXENB as *mut u32, 1);

        // Set AUX_MU_CNTL_REG to 0
        write_volatile(AUX_MU_CNTL_REG as *mut u32, 0);
        if int {
            // Set AUX_MU_IER_REG to 1 (enable receive interrupt)
            write_volatile(AUX_MU_IER_REG as *mut u32, 0b1);
            const IRQS1_ADDR: *mut u32 = 0x3f00b210 as *mut u32;
            const IRQS2_ADDR: *mut u32 = 0x3f00b214 as *mut u32;
            // Set IRQs1 register to enable mini UART interrupt
            write_volatile(IRQS1_ADDR, 1 << 29);
            // write_volatile(IRQS2_ADDR, 1 << 25);
        } else {
            // Set AUX_MU_IER_REG to 0 (disable interrupts)
            write_volatile(AUX_MU_IER_REG as *mut u32, 0);
        }

        // Set AUX_MU_LCR_REG to 3
        write_volatile(AUX_MU_LCR_REG as *mut u32, 3);
        // Set AUX_MU_MCR_REG to 0
        write_volatile(AUX_MU_MCR_REG as *mut u32, 0);
        // Set AUX_MU_BAUD_REG to 270
        write_volatile(AUX_MU_BAUD_REG as *mut u32, 270);
        // Set AUX_MU_IIR_REG to 6
        write_volatile(AUX_MU_IIR_REG as *mut u32, 6);
        // Set AUX_MU_CNTL_REG to 3
        write_volatile(AUX_MU_CNTL_REG as *mut u32, 3);
        READ_RING_BUFFER.head = 0;
        READ_RING_BUFFER.tail = 0;
        WRITE_RING_BUFFER.head = 0;
        WRITE_RING_BUFFER.tail = 0;
    }
}
pub struct Uart;

use core::fmt::{self, Write};

pub struct UartWriter;
pub struct UartWriterPolling;

// core::fmt::write needs a mutable reference to the writer
// we create a UartWriter which is implemented for Write trait with write_str method
// write_str method writes the formatted string to the buffer
impl Write for UartWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for i in s.bytes() {
            write_u8_buf(i);
        }
        write_int(true);
        Ok(())
    }
    fn write_fmt(&mut self, args: core::fmt::Arguments) -> core::fmt::Result {
        core::fmt::write(&mut UartWriter, args).unwrap();
        Ok(())
    }
}

impl Write for UartWriterPolling {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for i in s.bytes() {
            unsafe { write_u8(i) };
        }
        Ok(())
    }
    fn write_fmt(&mut self, args: core::fmt::Arguments) -> core::fmt::Result {
        core::fmt::write(&mut UartWriterPolling, args).unwrap();
        Ok(())
    }
}

impl UartWriterPolling {
    pub fn new() -> UartWriterPolling {
        UartWriterPolling
    }
}

impl UartWriter {
    pub fn new() -> UartWriter {
        UartWriter
    }
}

impl Uart {
    pub fn new() -> Uart {
        Uart
    }
}

#[no_mangle]
#[inline(never)]
pub fn uart_write_str(s: &str) {
    for i in s.bytes() {
        unsafe { write_u8(i) };
    }
}

#[no_mangle]
pub fn getline(s: &mut [u8; 128], is_echo: bool) -> &str {
    let mut ptr: usize = 0;
    unsafe {
        loop {
            let c = read_u8();
            match c {
                Some(i) => {
                    if is_echo {
                        write_u8(i as u8);
                    }
                    if i == 13 {
                        write_u8(10);
                        break;
                    }
                    s[ptr] = i as u8;
                    ptr = ptr + 1;
                }
                None => {}
            }
        }
    }
    core::str::from_utf8(&s[0..ptr]).unwrap()
}

#[no_mangle]
pub unsafe fn uart_nops() {
    for _ in 0..150 {
        asm!("nop");
    }
}

// Function to print something using the UART
#[no_mangle]
pub unsafe fn write_u8(s: u8) {
    loop {
        if (read_volatile(AUX_MU_LSR_REG as *mut u32) & 0b100000) != 0 {
            break;
        }
    }
    write_volatile(AUX_MU_IO_REG as *mut u8, s as u8);
}

#[no_mangle]
pub unsafe fn read_u8() -> Option<u8> {
    let lsr: u32 = read_volatile(AUX_MU_LSR_REG as *mut u32) & 0b1;
    if lsr != 0 {
        Some(read_volatile(AUX_MU_IO_REG as *mut u8))
    } else {
        None
    }
}

pub unsafe fn read(s: *mut u8, len: usize) {
    let mut ptr: usize = 0;
    while ptr < len {
        let c = read_u8();
        match c {
            Some(i) => {
                write_volatile(s.add(ptr), i as u8);
                ptr = ptr + 1;
            }
            None => {}
        }
    }
}

// print u32 in hex
#[no_mangle]
pub fn print_hex(n: u32) {
    let mut buf: [u8; 8] = [0; 8];
    let mut ptr: usize = 0;
    let mut num: u32 = n;
    for _ in 0..8 {
        let rem: u8 = (num % 16) as u8;
        if rem < 10 {
            buf[ptr] = (rem + b'0') as u8;
        } else {
            buf[ptr] = (rem - 10 + b'A') as u8;
        }
        ptr = ptr + 1;
        num = num / 16;
    }
    for i in buf.iter().take(8).rev() {
        unsafe {
            write_u8(*i);
        }
    }
}

pub fn strncmp(s1: &str, s2: &str, n: usize) -> bool {
    let mut i = 0;
    while i < n {
        if s1.as_bytes()[i] != s2.as_bytes()[i] {
            return false;
        }
        i = i + 1;
    }
    true
}

pub fn reboot() {
    const PM_PASSWORD: u32 = 0x5a000000;
    const PM_RSTC: u32 = 0x3F10001c;
    const PM_WDOG: u32 = 0x3F100024;
    const PM_RSTC_WRCFG_FULL_RESET: u32 = 0x00000020;
    unsafe {
        write_volatile(PM_WDOG as *mut u32, PM_PASSWORD | 100);
        write_volatile(PM_RSTC as *mut u32, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
    }
}
