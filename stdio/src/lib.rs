#![no_std]

pub mod macros;
use driver::uart;

fn send(c: u8) {
    uart::send(c);
    // uart::send_async(c);
}

fn _recv() -> u8 {
    loop {
        if let Some(c) = uart::recv_async() {
            return c;
        }
    }
}

fn recv() -> u8 {
    // let c = uart::recv();
    let c = _recv();
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
fn read(buf: &mut [u8]) {
    for i in buf.iter_mut() {
        *i = recv();
    }
}

#[allow(dead_code)]
fn write(buf: &[u8]) {
    for &c in buf {
        send(c);
    }
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
