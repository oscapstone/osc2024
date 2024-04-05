use crate::cpu::uart::{send, recv};

pub fn get_line(buf: &mut[u8], len: usize) -> usize {
    for idx in 0..len {
        let inp: u8;
        unsafe {
            inp = recv();
            send(inp);
        }
        buf[idx] = inp;
        if inp == b'\r' {
            unsafe {
                send(b'\n');
            }
            return idx + 1;
        }
    }
    len
}

pub fn print(s: &str) {
    for c in s.bytes() {
        unsafe {
            send(c);
        }
    }
}

pub fn println(s: &str) {
    print(s);
    unsafe {
        send(b'\r');
        send(b'\n');
    }
}

pub fn print_hex(n: u32) {
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
    println("");
}

pub fn print_dec(n: u32) {
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
            send(b'0');
        }
    } else {
        for i in (0..idx).rev() {
            unsafe {
                send(buf[i]);
            }
        }
    }
    println("");
}

pub fn print_hex_u64(n: u64) {
    for i in 0..16 {
        let shift = (15 - i) * 4;
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
    println("");
}

pub fn print_char(c: u8) {
    unsafe {
        send(c);
    }
}

pub fn print_char_64(chars: u64) {
    for i in (0..8).rev() {
        let c = (chars >> (i * 8)) as u8;
        print_char(c);
    }
    println("");
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