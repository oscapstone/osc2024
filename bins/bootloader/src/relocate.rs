use library::{console::console, println};

pub const RELOCATE_ADDR: *mut u8 = 0x2080000 as *mut u8;
pub const BINARY_START_ADDR: *mut u8 = 0x80000 as *mut u8;

pub struct Relocater {}

struct RelocaterProtocalHeader {
    size: u64,
}

impl Relocater {
    pub fn run(&self) {
        let relocater_protocal_header = self.read_header();
        println!(
            "Expected kernel size: {} bytes",
            relocater_protocal_header.size
        );
        self.relocate_kernel(relocater_protocal_header.size);
    }

    fn read_u64(&self) -> u64 {
        let mut n = 0;
        for i in 0..8 {
            let c = console().read_char_async();
            while c.is_none() {}
            n += (c.unwrap() as u64) << (i * 8);
        }
        n
    }

    fn read_header(&self) -> RelocaterProtocalHeader {
        RelocaterProtocalHeader {
            size: self.read_u64(),
        }
    }

    fn relocate_kernel(&self, size: u64) {
        for i in 0..size {
            unsafe {
                let c = console().read_char_async();
                while c.is_none() {}
                *(BINARY_START_ADDR.add(i as usize)) = c.unwrap() as u8;
            }
        }
    }
}
