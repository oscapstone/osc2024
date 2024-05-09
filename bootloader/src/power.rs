use core::ptr::write_volatile;

pub fn reboot() {
  reset(100);
}

const PM_PASSWORD: u32 = 0x5a000000;
const PM_RSTC: u32 = 0x3F10_001C;
const PM_WDOG: u32 = 0x3F10_0024;

#[allow(dead_code)]
pub fn reset(tick: u32) {
  unsafe {
      let mut r = PM_PASSWORD | 0x20;
      write_volatile(PM_RSTC as *mut u32, r);
      r = PM_PASSWORD | tick;
      write_volatile(PM_WDOG as *mut u32, r);
  }
}

#[allow(dead_code)]
pub fn cancel_reset() {
  unsafe {
      let r = PM_PASSWORD | 0;
      write_volatile(PM_RSTC as *mut u32, r);
      write_volatile(PM_WDOG as *mut u32, r);
  }
}