//! System console.

use crate::sync::Mutex;
mod null_console;

/// Console write functions.
pub trait Write {
    /// Write a single character.
    fn write_char(&self, c: char);

    /// Write a string.
    fn write_str(&self, s: &str) {
        for c in s.chars() {
            self.write_char(c)
        }
    }

    /// Write a Rust format string.
    fn write_fmt(&self, args: core::fmt::Arguments) -> core::fmt::Result;
}

/// Console read functions.
pub trait Read {
    /// Read a single character.
    fn read_char(&self) -> char {
        ' '
    }
}

/// Trait alias for a full-fledged console.
pub trait All: Write + Read {}

type Console = &'static (dyn All + Sync);

static CURRENT_CONSOLE: Mutex<Console> = Mutex::new(&null_console::NULL_CONSOLE);

pub fn register_console(new_console: Console) {
    let mut current_console = CURRENT_CONSOLE.lock().unwrap();
    *current_console = new_console;
}

pub fn console() -> Console {
    let current_console = CURRENT_CONSOLE.lock().unwrap();
    *current_console
}
