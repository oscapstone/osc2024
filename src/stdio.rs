use crate::uart;

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
pub fn getu32() -> u32 {
    let mut buf = [0u8; 11];
    let len = gets(&mut buf);
    let s = core::str::from_utf8(&buf[..len]).unwrap();
    s.trim().parse().unwrap()
}

#[allow(dead_code)]
pub fn putu32(mut n: u32) {
    let mut buf = [0u8; 11];
    let mut i = 10;
    loop {
        buf[i] = b'0' + (n % 10) as u8;
        n /= 10;
        if n == 0 {
            break;
        }
        i -= 1;
    }
    write(&buf[i..]);
}
