use alloc::string::String;

// read untill null byte
pub fn read_string(addr: u32) -> String {
    let mut addr = addr;
    let mut string = String::new();
    loop {
        let c = unsafe { core::ptr::read_volatile(addr as *const u8) };
        if c == 0 {
            break;
        }
        string.push(c as char);
        addr += 1;
    }
    string
}
