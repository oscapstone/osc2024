
use super::system_call::{uart_read, uart_write};

pub fn print(s: &str) {
    uart_write(s.as_bytes().as_ptr(), s.len());
}

pub fn println(s: &str) {
    print(s);
    print("\r\n");
}

#[no_mangle]
#[inline(never)]
pub fn print_hex(val: u64) {
    let mut buf = [0u8; 18];
    buf[16] = b'\r';
    buf[17] = b'\n';

    let mut idx = 0;
    for i in 0..16 {
        let nibble = (val >> (60 - i * 4)) & 0xF;
        buf[idx] = match nibble {
            0..=9 => (nibble + 48) as u8,
            10..=15 => (nibble + 87) as u8,
            _ => 0,
        };
        idx += 1;
    }

    uart_write(buf.as_ptr(), 18);
}

// // format println macro
// #[macro_export]
// macro_rules! println {
//     () => ($crate::os::stdio::println($str));
//     ($str:expr) => ($crate::os::stdio::println($str));
//     ($($arg:tt)*) => {{
//         $crate::os::stdio::println(alloc::format!($($arg)*).as_str());
//     }};
// }

// // format print macro
// #[macro_export]
// macro_rules! print {
//     () => ($crate::os::stdio::print($str));
//     ($str:expr) => ($crate::os::stdio::print($str));
//     ($($arg:tt)*) => {{
//         $crate::os::stdio::print(alloc::format!($($arg)*).as_str());
//     }};
// }