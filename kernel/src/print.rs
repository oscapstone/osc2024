// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

//! Printing.

use driver::uart;
use core::fmt;


pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    let mut writer = uart::UartWriter::new();
    writer.write_fmt(args);
}
#[no_mangle]
#[inline(never)]
pub fn _print_polling(args: fmt::Arguments) {
    use core::fmt::Write;
    let mut writer: uart::UartWriterPolling = uart::UartWriterPolling::new();
    writer.write_fmt(args);
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::print::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! print_polling {
    ($($arg:tt)*) => ($crate::print::_print_polling(format_args!($($arg)*)));
}


#[macro_export]
macro_rules! println {
    () => ($crate::print!("\r\n"));
    ($($arg:tt)*) => ({
        $crate::print::_print(format_args_nl!($($arg)*));
        $crate::print!("\r")
    })
}


#[macro_export]
macro_rules! println_polling {
    () => ($crate::print_polling!("\r\n"));
    ($($arg:tt)*) => ({
        $crate::print::_print_polling(format_args_nl!($($arg)*));
        $crate::print_polling!("\r")
    })
}
