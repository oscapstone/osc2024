use super::entry::Entry;
use super::page_table::PageTable;
use alloc::alloc::alloc;
use core::alloc::Layout;
use stdio::println;

#[derive(Debug, Clone)]
pub struct VirtualMemory {
    root: PageTable,
}

impl VirtualMemory {
    pub fn new() -> Self {
        VirtualMemory {
            root: PageTable::new(),
        }
    }

    pub fn get_l0_addr(&self) -> *mut u8 {
        self.root.addr as *mut u8
    }

    fn get_page(&mut self, addr: u64) -> &mut Entry {
        self.root.get_page(addr, 0)
    }

    pub fn mmap(&mut self, addr: u64, size: usize, flag: u32) -> *mut u8 {
        let flag = flag & 0xfff;
        let page = self.get_page(addr);
        let mem =
            unsafe { alloc(Layout::from_size_align(size as usize, 0x1000).unwrap()) as *mut u8 };
        println!(
            "mmap: addr: 0x{:x}, size: 0x{:x}, mem: 0x{:x}",
            addr, size, mem as u64
        );
        page.set_addr(mem as u32);
        page.set_flag(flag);
        return addr as *mut u8;
    }

    pub fn get_phys(&mut self, addr: u64) -> *mut u8 {
        let page = self.get_page(addr);
        page.get_addr()
    }
    pub fn dump(&self) {
        println!("VirtualMemory:");
        println!("  root: 0x{:x}", self.root.addr);
        unsafe { dump(self.root.addr as *mut u64, 0) }
    }
}

unsafe fn dump(addr: *mut u64, level: u8) {
    println!("  addr: 0x{:x}", addr as u64);
    for i in 0..512 {
        let p = addr.add(i);
        if *p & 0b1 == 1 {
            println!("    [{:03}] = 0x{:016x}", i, *p);
        }
    }
    for i in 0..512 {
        let p = addr.add(i);
        if *p & 0b1 == 1 {
            if *p & 0b11 == 0b11 {
                if level < 3 {
                    dump((*p & !0xfff) as *mut u64, level + 1);
                } else {
                    println!("    [{:03}] = 0x{:016x}", i, *p);
                }
            }
        }
    }
}
