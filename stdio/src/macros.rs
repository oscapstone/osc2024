use super::write;

pub struct UartWriter;

impl core::fmt::Write for UartWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        write(s.as_bytes());
        Ok(())
    }
}
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ({
        let _ = core::fmt::write(&mut stdio::macros::UartWriter, format_args!($($arg)*));
    });
}

#[macro_export]
macro_rules! println {
    () => (stdio::print!("\r\n"));
    ($($arg:tt)*) => ({
        stdio::print!($($arg)*);
        stdio::print!("\r\n");
    });
}
