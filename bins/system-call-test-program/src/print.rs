use core::fmt::{Arguments, Write};

use crate::console::console;

pub fn _print(args: Arguments) {
    console().write_fmt(args).unwrap();
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::print::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\r\n"));
    ($($arg:tt)*) => ({
        $crate::print!("{}\r\n", format_args!($($arg)*));
    })
}
