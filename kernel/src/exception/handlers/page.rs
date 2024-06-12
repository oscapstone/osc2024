use core::arch::asm;
use stdio::println;

pub unsafe fn page_fault() {
    let esr_el1: u64;
    asm!(
        "mrs {0}, esr_el1",
        out(reg) esr_el1,
    );
    let ec = esr_el1 >> 26;
    let far_el1: u64;
    asm!(
        "mrs {0}, far_el1",
        out(reg) far_el1,
    );
    println!("Page fault");
    println!("Exception Class: 0b{:06b}", ec);
    println!("ESR_EL1: 0x{:x}", esr_el1);
    println!("FAR_EL1: 0x{:x}", far_el1);
    panic!("Page fault");
}
