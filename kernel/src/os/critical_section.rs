use core::arch::asm;
use core::ptr::{read_volatile, write_volatile};

const CS_COUNTER: *mut u32 = 0x7_5000 as *mut u32;

#[no_mangle]
#[inline(never)]
pub extern "C" fn disable_irq() {
    unsafe {
        let mut counter: u32 = read_volatile(CS_COUNTER);
        counter += 1;
        write_volatile(CS_COUNTER, counter);
        asm!("msr DAIFSet, 0xf");
    }
}
#[no_mangle]
#[inline(never)]
pub extern "C" fn enable_irq() {
    unsafe {
        let mut counter: u32 = read_volatile(CS_COUNTER);
        counter -= 1;
        write_volatile(CS_COUNTER, counter);
        if counter == 0 {
            asm!("msr DAIFClr, 0xf");
        }
    }
}
