#![no_std]
#![no_main]

use core::arch::global_asm;
use panic_wait as _;

global_asm!(include_str!("boot.s"));
