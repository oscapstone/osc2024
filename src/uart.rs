use core::arch::asm;
use core::{
    ptr::{read_volatile, write_volatile},
    usize,
};

const AUXENB: u32 = 0x3F215004;
const AUX_MU_CNTL_REG: u32 = 0x3F215060;
const AUX_MU_IER_REG: u32 = 0x3F215044;
const AUX_MU_LCR_REG: u32 = 0x3F21504C;
const AUX_MU_MCR_REG: u32 = 0x3F215050;
const AUX_MU_BAUD_REG: u32 = 0x3F215068;
const AUX_MU_IIR_REG: u32 = 0x3F215048;
const AUX_MU_IO_REG: u32 = 0x3F215040;
const AUX_MU_LSR_REG: u32 = 0x3F215054;

const GPFSEL_BASE: u32 = 0x3F200000;
const GPFSEL0: u32 = 0x3F200000;
const GPFSEL1: u32 = 0x3F200004;

const GPPUD: u32 = 0x3F200094;
const GPPUDCLK0: u32 = 0x3F200098;
const GPPUDCLK1: u32 = 0x3F20009C;

// Initialize the UART
#[no_mangle]
#[inline(never)]
pub fn init_uart() {
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
        nops();

        // configure pull up/down clock register to disable GPIO pull up/down
        let pudclk0 = !(0b11 << 14);
        write_volatile(GPPUDCLK0 as *mut u32, pudclk0);
        let pudclk1 = 0;
        write_volatile(GPPUDCLK1 as *mut u32, pudclk1);
        // wait 150 cycles
        nops();

        // Write to GPPUD to remove the control signal
        write_volatile(GPPUD as *mut u32, 0);
        // Write to GPPUDCLK0 to remove the clock
        write_volatile(GPPUDCLK0 as *mut u32, 0);

        // write some word to uart to initialize it
        // Set AUXENB register to enable mini UART
        write_volatile(AUXENB as *mut u32, 1);

        // Set AUX_MU_CNTL_REG to 0
        write_volatile(AUX_MU_CNTL_REG as *mut u32, 0);
        // Set AUX_MU_IER_REG to 0
        write_volatile(AUX_MU_IER_REG as *mut u32, 0);
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
    }
}

#[no_mangle]
#[inline(never)]
pub fn print_hello() {
    unsafe {
        let hello: [u8; 5] = [72, 101, 108, 108, 111];
        write_volatile(AUX_MU_IO_REG as *mut u32, 72);
        write_volatile(AUX_MU_IO_REG as *mut u32, 101);
        write_volatile(AUX_MU_IO_REG as *mut u32, 108);
        write_volatile(AUX_MU_IO_REG as *mut u32, 108);
        write_volatile(AUX_MU_IO_REG as *mut u32, 111);
    }
}

#[no_mangle]
#[inline(never)]
pub unsafe fn print_str(buf: &[u8; 128], buf_len: usize) {
    let mut ptr: usize = 0;
    while ptr < buf_len {
        write_char(buf[ptr]);
        ptr = ptr + 1;
    }
}

#[no_mangle]
#[inline(never)]
pub unsafe fn strcmp(s1: &[u8; 128], l1: usize, s2: &[u8; 128], l2: usize) -> bool {
    if l1 != l2 {
        return false;
    }
    let mut idx: usize = 0;
    while idx < l1 {
        if s1[idx] != s2[idx] {
            return false;
        }
        idx = idx + 1;
    }
    true
}

#[no_mangle]
#[inline(never)]
pub unsafe fn get_line(s: &mut [u8; 128], is_echo: bool) -> usize {
    let mut ptr: usize = 0;
    loop {
        let c: Option<u32> = read_char();
        match c {
            Some(i) => {
                if is_echo {
                    write_char(i as u8);
                }
                if i == 13 {
                    write_char(10);
                    break;
                }
                s[ptr] = i as u8;
                ptr = ptr + 1;
            }
            None => {}
        }
        asm!("nop");
    }
    ptr
}

#[no_mangle]
pub unsafe fn nops() {
    for _ in 0..150 {
        asm!("nop");
    }
}

// Function to print something using the UART
#[no_mangle]
pub unsafe fn write_char(s: u8) {
    // Add your UART printing code here
    // Example: write to UART buffer or transmit data
    // check if the UART is ready to transmit
    loop {
        if (read_volatile(AUX_MU_LSR_REG as *mut u32) & 0b100000) != 0 {
            break;
        }
    }
    write_volatile(AUX_MU_IO_REG as *mut u8, s as u8);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn read_char() -> Option<u32> {
    let lsr: u32 = read_volatile(AUX_MU_LSR_REG as *mut u32) & 0b1;
    if lsr != 0 {
        Some(read_volatile(AUX_MU_IO_REG as *mut u32))
    } else {
        None
    }
}

// // Create a global instance of the UART
// pub static mut UART: Uart = Uart {};

// // UART struct
// pub struct Uart {}

// // Example usage
// fn main() {
//     unsafe {
//         init_uart();
//         writeln!(UART, "Hello, world!").unwrap();
//     }
// }
