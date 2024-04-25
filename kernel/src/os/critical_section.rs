use core::arch::asm;
use core::ptr::{read_volatile, write_volatile};

const CS_COUNTER: *mut u32 = 0x7_5000 as *mut u32;

pub fn disable_irq() {
    unsafe {
        let mut counter: u32 = read_volatile(CS_COUNTER);
        counter += 1;
        asm!("msr DAIFSet, 0xf");
        write_volatile(CS_COUNTER, counter);
    }
}
pub fn enable_irq() {
    unsafe {
        let mut counter: u32 = read_volatile(CS_COUNTER);
        counter -= 1;
        if counter == 1 {
            asm!("msr DAIFClr, 0xf");
        }
        write_volatile(CS_COUNTER, counter);
    }
}
