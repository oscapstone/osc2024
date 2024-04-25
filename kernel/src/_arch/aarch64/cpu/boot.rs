use core::arch::global_asm;

global_asm!(
    include_str!("boot.s") ,
    CONST_CORE_ID_MASK = const 0b11
);

global_asm!(include_str!("interrupt.s"));

// mod uart;

#[no_mangle]
pub unsafe fn _start_rust() {
    crate::kernel_init()
}
