use core::fmt;

const QEMU_OUTPUT_DEVICE: *mut u8 = 0x3F20_1000 as *mut u8;

struct QEMUOutput;

impl fmt::Write for QEMUOutput {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for c in s.chars() {
            unsafe {
                core::ptr::write_volatile(QEMU_OUTPUT_DEVICE, c as u8);
            }
        }

        Ok(())
    }
}

/// Return a reference to the console.
pub fn console() -> impl crate::console::Write {
    QEMUOutput {}
}
