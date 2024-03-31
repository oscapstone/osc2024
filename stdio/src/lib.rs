#![no_std]

use driver::uart;

fn send(c: u8) {
    uart::send(c);
}

fn recv() -> u8 {
    let c = uart::recv();
    match c {
        b'\r' | b'\n' => {
            write(b"\r\n");
            b'\n'
        }
        b'\x7f' | b'\x08' => {
            write(b"\x08 \x08");
            b'\x7f'
        }
        _ => {
            send(c);
            c
        }
    }
}

#[allow(dead_code)]
pub fn read(buf: &mut [u8]) {
    for i in buf.iter_mut() {
        *i = recv();
    }
}

#[allow(dead_code)]
pub fn write(buf: &[u8]) {
    for &c in buf {
        send(c);
    }
}

#[allow(dead_code)]
pub fn puts(buf: &[u8]) {
    for &c in buf {
        if c == 0 {
            break;
        }
        send(c);
    }
    write(b"\r\n".as_ref());
}

#[allow(dead_code)]
pub fn gets(buf: &mut [u8]) -> usize {
    let mut i = 0;
    loop {
        let input = recv();
        match input {
            b'\n' => {
                if i < buf.len() {
                    buf[i] = 0;
                    break;
                }
            }
            b'\x7f' => {
                if i < buf.len() && i > 0 {
                    i -= 1;
                    buf[i] = 0;
                }
            }
            _ => {
                if i < buf.len() {
                    buf[i] = input;
                    i += 1;
                }
            }
        }
    }
    i
}

#[allow(dead_code)]
pub fn print(buf: &str) {
    write(buf.as_bytes());
}

#[allow(dead_code)]
pub fn println(buf: &str) {
    puts(buf.as_bytes());
}

#[allow(dead_code)]
pub fn print_u32(val: u32) {
    let hex = u32_to_hex(val);
    write(&hex);
}

#[allow(dead_code)]
pub fn print_u64(val: u64) {
    let hex = u64_to_hex(val);
    write(&hex);
}

#[allow(dead_code)]
pub fn u32_to_hex(val: u32) -> [u8; 10] {
    let mut hex = [0; 10];
    hex[0] = b'0';
    hex[1] = b'x';
    for i in 0..8 {
        let nibble = (val >> (28 - i * 4)) & 0xF;
        hex[i + 2] = if nibble < 10 {
            nibble as u8 + b'0'
        } else {
            nibble as u8 + b'A' - 10
        };
    }
    hex
}

#[allow(dead_code)]
pub fn u64_to_hex(val: u64) -> [u8; 18] {
    let mut hex = [0; 18];
    hex[0] = b'0';
    hex[1] = b'x';
    for i in 0..16 {
        let nibble = (val >> (60 - i * 4)) & 0xF;
        hex[i + 2] = if nibble < 10 {
            nibble as u8 + b'0'
        } else {
            nibble as u8 + b'A' - 10
        };
    }
    hex
}

#[allow(dead_code)]
pub fn atoi(s: &[u8]) -> u32 {
    let mut val = 0;
    for &c in s {
        if c < b'0' || c > b'9' {
            break;
        }
        val = val * 10 + (c - b'0') as u32;
    }
    val
}
