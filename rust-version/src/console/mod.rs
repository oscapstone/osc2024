// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>
//! System console.

// use crate::{synchronization, synchronization::NullLock};
use crate::{synchronization, synchronization::NullLock};
use core::fmt;

//--------------------------------------------------------------------------------------------------
// Global instances
//--------------------------------------------------------------------------------------------------

static QEMU_OUTPUT: QEMUOutput = QEMUOutput::new();

//--------------------------------------------------------------------------------------------------
// Private Definitions
//--------------------------------------------------------------------------------------------------

/// A mystical, magical device for generating QEMU output out of the void.
///
/// The mutex protected part.
struct QEMUOutputInner {
    chars_written: usize,
}

/// A mystical, magical device for generating QEMU output out of the void.
struct QEMUOutput {
    inner: NullLock<QEMUOutputInner>,
}

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

/// Console interfaces.
pub mod interface {
    use core::fmt;
    /// Console write functions.
    pub trait Write {
        /// Write a Rust format string.
        fn write_fmt(&self, args: fmt::Arguments) -> fmt::Result;
    }

    /// Console statistics.
    pub trait Statistics {
        /// Return the number of characters written.
        fn chars_written(&self) -> usize {
            0
        }
    }

    /// Trait alias for a full-fledged console.
    pub trait All: Write + Statistics {}
}

//--------------------------------------------------------------------------------------------------
// Private Code
//--------------------------------------------------------------------------------------------------

impl QEMUOutputInner {
    const fn new() -> QEMUOutputInner {
        QEMUOutputInner { chars_written: 0 }
    }

    /// Send a character.
    fn write_char(&mut self, c: char) {
        unsafe {
            core::ptr::write_volatile(0x3F20_1000 as *mut u8, c as u8);
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
impl fmt::Write for QEMUOutputInner {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        for c in s.chars() {
            // Convert newline to carriage return + newline.
            if c == '\n' {
                self.write_char('\r')
            }

            self.write_char(c);
        }

        Ok(())
    }
}

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

impl QEMUOutput {
    /// Create a new instance.
    pub const fn new() -> QEMUOutput {
        QEMUOutput {
            inner: NullLock::new(QEMUOutputInner::new()),
        }
    }
}

/// Return a reference to the console.
///
/// This is the global console used by all printing macros.
pub fn console() -> &'static dyn interface::All {
    &QEMU_OUTPUT
}

//------------------------------------------------------------------------------
// OS Interface Code
//------------------------------------------------------------------------------
use synchronization::interface::Mutex;

/// Passthrough of `args` to the `core::fmt::Write` implementation, but guarded by a Mutex to
/// serialize access.
impl interface::Write for QEMUOutput {
    fn write_fmt(&self, args: core::fmt::Arguments) -> fmt::Result {
        // Fully qualified syntax for the call to `core::fmt::Write::write_fmt()` to increase
        // readability.
        self.inner.lock(|inner| fmt::Write::write_fmt(inner, args))
    }
}

impl interface::Statistics for QEMUOutput {
    fn chars_written(&self) -> usize {
        self.inner.lock(|inner| inner.chars_written)
    }
}

impl interface::All for QEMUOutput {}
