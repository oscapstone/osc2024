//! A small standard library with the needed functionalities.

#![no_std]

pub mod fmt;
pub mod string;
pub mod sync;
pub mod vec;

pub use fmt::_print;
