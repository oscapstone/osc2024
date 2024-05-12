
use super::system_call::{uart_read, uart_write};

pub fn print(s: &str) {
    uart_write(s.as_bytes().as_ptr(), s.len());
}

pub fn println(s: &str) {
    print(s);
    print("\r\n");
}

fn print_hex(val: u64) {
    let mut buf = [0u8; 16];
    
    for i in 0..16 {
        let shift = (15 - i) * 4;
        let digit = (val >> shift) & 0xF;
        let ascii = if digit < 10 {
            b'0' + digit as u8
        } else {
            b'A' + (digit - 10) as u8
        };
        buf[i] = ascii;
    }

    for i in 0..16 {
        uart_write(&buf[i], 1);
    }
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