use core::slice;

use library::console::console;

pub fn uart_read(buf: *mut u8, size: usize) -> usize {
    let mut buffer = unsafe { slice::from_raw_parts_mut(buf, size) };
    let mut i = 0;
    while let Some(c) = console().read_char() {
        buffer[i] = c as u8;
        i += 1;
        if i == size {
            break;
        }
    }
    i
}
