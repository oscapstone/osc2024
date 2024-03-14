//! System console.

/// Console write functions.
pub trait Write {
    /// Write a Rust format string.
    fn write_fmt(&self, args: core::fmt::Arguments) -> core::fmt::Result;
}

/// Console statistics.
pub trait Statistics {
    /// Return the number of characters written to the console.
    fn chars_written(&self) -> usize {
        0
    }
}

/// Trait alias for a full-fledged console.
pub trait All: Write + Statistics {}

mod qemu_output;

/// Return a reference to the console.
///
/// This is the global console used by all printing macros.
pub fn console() -> &'static dyn All {
    qemu_output::console()
}
