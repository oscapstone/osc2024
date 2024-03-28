#[no_mangle]
pub extern "C" fn memset(s: *mut u8, c: u8, n: usize) {
    for i in 0..n {
        unsafe {
            *s.add(i) = c;
        }
    }
}

#[no_mangle]
pub extern "C" fn memcpy(dest: *mut u8, src: *const u8, n: usize) {
    for i in 0..n {
        unsafe {
            *dest.add(i) = *src.add(i);
        }
    }
}

#[no_mangle]
pub extern "C" fn memcmp(s1: *const u8, s2: *const u8, n: usize) -> i32 {
    for i in 0..n {
        let c1 = unsafe { *s1.add(i) };
        let c2 = unsafe { *s2.add(i) };
        if c1 != c2 {
            return c1 as i32 - c2 as i32;
        }
    }
    0
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
