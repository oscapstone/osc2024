#[no_mangle]
pub extern "C" fn memset(s: *mut u8, c: u8, n: usize) {
    for i in 0..n {
        unsafe {
            *s.add(i) = c;
        }
    }
}

#[allow(dead_code)]
pub fn strlen(s: *const u8) -> usize {
    let mut len = 0;
    unsafe {
        while *s.add(len) != 0 {
            len += 1;
        }
    }
    len
}

#[allow(dead_code)]
pub fn strcpy(dest: *mut u8, src: *const u8) {
    let mut i = 0;
    unsafe {
        while *src.add(i) != 0 {
            *dest.add(i) = *src.add(i);
            i += 1;
        }
        *dest.add(i) = 0;
    }
}

#[allow(dead_code)]
pub fn strcmp(s1: *const u8, s2: *const u8) -> i32 {
    let mut i = 0;
    unsafe {
        while *s1.add(i) != 0 && *s2.add(i) != 0 {
            if *s1.add(i) != *s2.add(i) {
                return *s1.add(i) as i32 - *s2.add(i) as i32;
            }
            i += 1;
        }
    }
    0
}

#[allow(dead_code)]
pub fn strncmp(s1: *const u8, s2: *const u8, n: usize) -> i32 {
    let mut i = 0;
    unsafe {
        while i < n {
            if *s1.add(i) != *s2.add(i) {
                return *s1.add(i) as i32 - *s2.add(i) as i32;
            }
            i += 1;
        }
    }
    0
}

#[allow(dead_code)]
pub fn to_hex(val: u32) -> [u8; 10] {
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
