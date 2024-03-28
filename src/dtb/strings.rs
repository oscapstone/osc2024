use alloc::string::String;
use stdio;

pub struct StringMap {
    base: u32,
}

impl StringMap {
    pub fn load(base: u32) -> StringMap {
        StringMap { base }
    }

    pub fn get(&self, offset: u32) -> String {
        let mut addr = self.base + offset;
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

    #[allow(dead_code)]
    pub fn print(&self) {
        let mut offset = 0;
        loop {
            let string = self.get(offset);
            if string.is_empty() {
                break;
            }
            stdio::print("String: ");
            stdio::print(&string);
            stdio::println("");
            offset += string.len() as u32 + 1;
        }
    }
}
