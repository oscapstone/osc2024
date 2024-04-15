use alloc::string::ToString;

use crate::cpu::uart::{send, recv, send_async, recv_async};
use core::arch::asm;
use super::critical_section::{disable_irq, enable_irq};

pub fn get_line(buf: &mut[u8], len: usize) -> usize {
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
                    },
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

fn print_hex(n: u32) {
    for i in 0..8 {
        let shift = (7 - i) * 4;
        let digit = (n >> shift) & 0xF;
        let ascii = if digit < 10 {
            b'0' + digit as u8
        } else {
            b'A' + (digit - 10) as u8
        };
        unsafe {
            send_async(ascii);
        }
    }
    println("");
}

fn print_dec(n: u32) {
    let mut n = n;
    let mut buf = [0u8; 10];
    let mut idx = 0;
    while n > 0 {
        buf[idx] = b'0' + (n % 10) as u8;
        n /= 10;
        idx += 1;
    }
    if idx == 0 {
        unsafe {
            send_async(b'0');
        }
    } else {
        for i in (0..idx).rev() {
            unsafe {
                send_async(buf[i]);
            }
        }
    }
    println("");
}

fn print_hex_u64(n: u64) {
    for i in 0..16 {
        let shift = (15 - i) * 4;
        let digit = (n >> shift) & 0xF;
        let ascii = if digit < 10 {
            b'0' + digit as u8
        } else {
            b'A' + (digit - 10) as u8
        };
        unsafe {
            send_async(ascii);
        }
    }
    println("");
}

fn print_char(c: u8) {
    unsafe {
        send_async(c);
    }
}

fn print_char_64(chars: u64) {
    for i in (0..8).rev() {
        let c = (chars >> (i * 8)) as u8;
        print_char(c);
    }
    println("");
}

fn println_async(s: &str) {
    // println(s);
    // print_dec(s.len() as u32);
    for c in s.bytes() {
        // print_hex(c as u32);
        unsafe {
            send_async(c);
        }
    }
    unsafe {
        send_async(b'\r');
        send_async(b'\n');
    }
}

pub fn println_now(s: &str) {
    for c in s.bytes() {
        unsafe {
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