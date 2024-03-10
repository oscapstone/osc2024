#[no_mangle]
pub extern "C" fn memset(s: *mut u8, c: u8, n: usize) {
    for i in 0..n {
        unsafe {
            *s.add(i) = c;
        }
    }
}
