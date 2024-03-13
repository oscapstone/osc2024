use crate::uart::recv;
use crate::uart::send;

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
