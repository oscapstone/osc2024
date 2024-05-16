use aarch64_cpu::{
    asm::{self, eret},
    registers::*,
};

#[inline(always)]
pub fn spin_for_cycle(cycle: usize) {
    for _ in 0..cycle {
        asm::nop();
    }
}

#[inline(always)]
pub unsafe fn switch_to_el1(stack_end: u64, kernel_init_fn: unsafe fn() -> !) -> ! {
    HCR_EL2.write(HCR_EL2::RW::EL1IsAarch64);
    SPSR_EL2.write(
        SPSR_EL2::D::Masked
            + SPSR_EL2::A::Masked
            + SPSR_EL2::I::Masked
            + SPSR_EL2::F::Masked
            + SPSR_EL2::M::EL1h,
    );
    ELR_EL2.set(kernel_init_fn as *const fn() -> ! as u64);
    SP_EL1.set(stack_end);
    eret();
}

#[inline(always)]
pub unsafe fn run_user_code(stack_end: u64, code_start: u64) {
    SPSR_EL1.write(
        SPSR_EL1::D::Masked
            + SPSR_EL1::A::Masked
            + SPSR_EL1::I::Unmasked
            + SPSR_EL1::F::Masked
            + SPSR_EL1::M::EL0t,
    );
    ELR_EL1.set(code_start);
    SP_EL0.set(stack_end);
    eret();
}

#[inline(always)]
pub unsafe fn enable_kernel_space_interrupt() {
    DAIF.write(DAIF::I::Unmasked);
}

#[inline(always)]
pub unsafe fn disable_kernel_space_interrupt() {
    DAIF.write(DAIF::I::Masked);
}

#[inline(always)]
pub fn current_task() -> u64 {
    TPIDR_EL1.get()
}
