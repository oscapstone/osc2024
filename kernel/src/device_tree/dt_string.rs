extern crate alloc;
use alloc::string::String;

pub struct DtString {
    base_address: u32,
}

impl DtString {
    pub fn load(base_address: u32) -> DtString {
        DtString { base_address }
    }

    // get string end with '\0'
    pub fn get(&self, offset: u32) -> String {
        let mut addr = self.base_address + offset;
        let mut string = String::new();
        //println!("[Device Tree Dtstring] String: {} Addr: {:#x}", string ,addr);
        loop {
            let c = unsafe { core::ptr::read_volatile(addr as *const u8) };
            if c == 0 {
                break;
            }
            string.push(c as char);
            addr += 1;
        }
        //println!("[Device Tree Dtstring] String: {} Addr: {:#x}", string ,addr);
        string
    }
}

// get string end with '\0'
pub fn load_string(base_address: u32) -> String {
    let mut addr = base_address;
    let mut string = String::new();
    //println!("[Device Tree load_string] String: {} Addr: {:#x}", string ,addr);
    loop {
        let c = unsafe { core::ptr::read_volatile(addr as *const u8) };
        if c == 0 {
            break;
        }
        string.push(c as char);
        addr += 1;
    }
    //println!("[Device Tree load_string] String: {} Addr: {:#x}", string ,addr);
    string
}
