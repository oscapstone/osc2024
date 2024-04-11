
use core::arch::asm;


pub fn disable_irq() {
    unsafe {
        let mut counter: u32 = *(0x75000 as *mut u32);
        counter += 1;
        asm!("msr DAIFSet, 0xf");
    }
}
pub fn enable_irq() {
    unsafe {
        let mut counter: u32 = *(0x75000 as *mut u32);
        counter -= 1;
        if counter == 1 {
            asm!("msr DAIFClr, 0xf");
        }
    }
}