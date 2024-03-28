
use core::arch::global_asm;
use core::arch::asm;

use driver::uart;

global_asm!(
    include_str!("boot.s") ,
    CONST_CORE_ID_MASK = const 0b11
);

// mod uart;

#[no_mangle]
pub unsafe fn _start_rust(){
    crate::bootloader();
    uart::_print("Bootloader finished\r\n");
}
