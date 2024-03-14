use crate::sync::Mutex;

struct QEMUOutputInner {
    chars_written: usize,
}

pub struct QEMUOutput {
    inner: Mutex<QEMUOutputInner>,
}

static QEMU_OUTPUT: QEMUOutput = QEMUOutput::new();

impl QEMUOutputInner {
    const QEMU_OUTPUT_DEVICE: *mut u8 = 0x3F20_1000 as *mut u8;

    const fn new() -> Self {
        Self { chars_written: 0 }
    }

    fn write_char(&mut self, c: char) {
        unsafe {
            core::ptr::write_volatile(Self::QEMU_OUTPUT_DEVICE, c as u8);
        }

        self.chars_written += 1;
    }
}

/// Implementing `core::fmt::Write` enables usage of the `format_args!` macros, which in turn are
/// used to implement the `kernel`'s `print!` and `println!` macros. By implementing `write_str()`,
/// we get `write_fmt()` automatically.
///
/// The function takes an `&mut self`, so it must be implemented for the inner struct.
///
/// See [`src/print.rs`].
///
/// [`src/print.rs`]: ../../print/index.html
impl core::fmt::Write for QEMUOutputInner {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for c in s.chars() {
            // Convert LF to CRLF
            if c == '\n' {
                self.write_char('\r');
            }
            self.write_char(c);
        }

        Ok(())
    }
}

impl QEMUOutput {
    pub const fn new() -> Self {
        Self {
            inner: Mutex::new(QEMUOutputInner::new()),
        }
    }
}

/// Return a reference to the console.
pub fn console() -> &'static dyn super::All {
    &QEMU_OUTPUT
}

impl super::Write for QEMUOutput {
    fn write_fmt(&self, args: core::fmt::Arguments) -> core::fmt::Result {
        let mut inner = self.inner.lock().unwrap();
        core::fmt::Write::write_fmt(&mut *inner, args)
    }
}

impl super::Statistics for QEMUOutput {
    fn chars_written(&self) -> usize {
        let inner = self.inner.lock().unwrap();
        inner.chars_written
    }
}

impl super::All for QEMUOutput {}
