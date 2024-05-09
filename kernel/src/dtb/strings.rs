use super::utils::read_string;
use alloc::string::String;
pub struct StringMap {
    base: u32,
}

impl StringMap {
    pub fn load(base: u32) -> StringMap {
        StringMap { base }
    }

    pub fn get(&self, offset: u32) -> String {
        read_string(self.base + offset)
    }
}
