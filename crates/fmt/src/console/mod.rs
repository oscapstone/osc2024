//! System console.

pub use core::fmt::Write;

mod qemu_output;

/// Return a reference to the console.
///
/// This is the global console used by all printing macros.
pub fn console() -> impl Write {
    qemu_output::console()
}
