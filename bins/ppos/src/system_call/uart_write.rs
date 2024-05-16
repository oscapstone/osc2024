use core::slice;

use library::print;

pub fn uart_write(buf: *const u8, size: usize) -> usize {
    let buffer = unsafe { slice::from_raw_parts(buf, size) };
    for c in buffer {
        print!("{}", *c as char);
    }
    size
}
