use super::critical_section::{disable_irq, enable_irq};
use crate::cpu::uart::{recv, recv_async, send, send_async};
use core::arch::asm;

pub fn get_line(buf: &mut [u8], len: usize) -> usize {
    for c in buf.iter_mut() {
        *c = 0;
    }

    for idx in 0..len {
        let inp: u8;
        unsafe {
            loop {
                asm!("msr DAIFSet, 0xf");
                let recv = recv_async();
                asm!("msr DAIFClr, 0xf");
                match recv {
                    Some(c) => {
                        inp = c;
                        break;
                    }
                    None => {
                        continue;
                    }
                };
            }
            send_async(inp);
        }

        buf[idx] = inp;
        if inp == b'\r' {
            unsafe {
                send_async(b'\n');
            }
            return idx + 1;
        }
    }
    len
}

pub fn print(s: &str) {
    for c in s.bytes() {
        unsafe {
            disable_irq();
            send_async(c);
            enable_irq();
        }
    }
}

pub fn println(s: &str) {
    print(s);
    print("\r\n");
}

pub fn print_hex_now(n: u32) {
    for i in 0..8 {
        let shift = (7 - i) * 4;
        let digit = (n >> shift) & 0xF;
        let ascii = if digit < 10 {
            b'0' + digit as u8
        } else {
            b'A' + (digit - 10) as u8
        };
        unsafe {
            send(ascii);
        }
    }
    println_now("");
}

pub fn println_now(s: &str) {
    for c in s.bytes() {
        unsafe {
            if c == b'\n' {
                send(b'\r');
            }
            send(c);
        }
    }
    unsafe {
        send(b'\r');
        send(b'\n');
    }
}

// format println macro
#[macro_export]
macro_rules! println {
    () => ($crate::os::stdio::println($str));
    ($str:expr) => ($crate::os::stdio::println($str));
    ($($arg:tt)*) => {{
        $crate::os::stdio::println(alloc::format!($($arg)*).as_str());
    }};
}

// format print macro
#[macro_export]
macro_rules! print {
    () => ($crate::os::stdio::print($str));
    ($str:expr) => ($crate::os::stdio::print($str));
    ($($arg:tt)*) => {{
        $crate::os::stdio::print(alloc::format!($($arg)*).as_str());
    }};
}
