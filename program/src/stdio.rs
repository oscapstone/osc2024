use driver::uart::send;

pub fn print(s: &str) {
    for c in s.chars() {
        send(c as u8);
    }
}

pub fn println(s: &str) {
    print(s);
    send(b'\r');
    send(b'\n');
}

pub fn print_u64(name: &str, n: u64) {
    print(name);
    print(": ");
    print_dec(n);
    print(" (0x");
    print_hex(n);
    print(")");
    send(b'\r');
    send(b'\n');
}

pub fn print_hex(n: u64) {
    let mut s = [0u8; 16];
    let mut i = 0;
    let mut n = n;
    while n > 0 {
        let d = n % 16;
        s[i] = if d < 10 {
            b'0' + d as u8
        } else {
            b'a' + (d - 10) as u8
        };
        n /= 16;
        i += 1;
    }
    if i == 0 {
        send(b'0');
    }
    for j in (0..i).rev() {
        send(s[j]);
    }
}

fn print_dec(n: u64) {
    let mut s = [0u8; 20];
    let mut i = 0;
    let mut n = n;
    while n > 0 {
        let d = n % 10;
        s[i] = b'0' + d as u8;
        n /= 10;
        i += 1;
    }
    if i == 0 {
        send(b'0');
    }
    for j in (0..i).rev() {
        send(s[j]);
    }
}

fn print_bin(n: u64) {
    let mut s = [0u8; 64];
    let mut i = 0;
    let mut n = n;
    while n > 0 {
        s[i] = b'0' + (n & 1) as u8;
        n >>= 1;
        i += 1;
    }
    if i == 0 {
        send(b'0');
    }
    for j in (0..i).rev() {
        send(s[j]);
    }
}
