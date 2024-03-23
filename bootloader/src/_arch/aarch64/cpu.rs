use aarch64_cpu::asm;

// public code
/// pause execution on the core


#[inline(always)]
pub fn wait_forever() -> ! {
    loop {
        asm::wfe();
    }
}

/*
const int PM_RSTC = 0x2010001c;
const int PM_WDOG = 0x20100024;
const int PM_PASSWORD = 0x5a000000;
const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;

PUT32(PM_WDOG, PM_PASSWORD | 1); // timeout = 1/16th of a second? (whatever)
PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
*/


