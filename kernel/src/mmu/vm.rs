use super::entry::Entry;
use super::page_table::PageTable;
use alloc::alloc::alloc;
use core::alloc::Layout;
use stdio::println;

#[derive(Debug)]
pub struct VirtualMemory {
    root: PageTable,
}

impl VirtualMemory {
    pub fn new() -> Self {
        VirtualMemory {
            root: PageTable::new(),
        }
    }

    pub fn load(addr: u64) -> Self {
        // println!("VirtualMemory::load: 0x{:x}", addr);
        VirtualMemory {
            root: PageTable::load(addr, 0),
        }
    }

    pub fn get_l0_addr(&self) -> *mut u8 {
        self.root.addr as *mut u8
    }

    fn get_page(&self, addr: u64) -> &Entry {
        self.root.get_page(addr, 0)
    }

    fn create_page(&mut self, addr: u64) -> &mut Entry {
        self.root.create_page(addr, 0)
    }

    pub fn map_pa(&mut self, addr: u64, pa: u64, size: usize, flag: u32) -> *mut u8 {
        println!(
            "map_pa: 0x{:x} -> 0x{:x}, size: 0x{:x}, flag: 0x{:x}",
            addr, pa, size, flag
        );
        let flag = flag & 0xfff;
        for i in (0..size).step_by(0x1000) {
            let page = self.create_page(addr + i as u64);
            page.set_addr((pa + i as u64) as u32);
            page.set_flag(flag);
        }
        println!("map_pa: 0x{:x} -> 0x{:x}", addr, pa);
        return addr as *mut u8;
    }

    pub fn mmap(&mut self, addr: u64, size: usize, flag: u32) -> *mut u8 {
        println!("mmap: 0x{:x}, size: 0x{:x}, flag: 0x{:x}", addr, size, flag);
        let flag = flag & 0xfff;
        let mem = unsafe { alloc(Layout::from_size_align(size, 0x1000).unwrap()) as *mut u8 };
        for i in (0..size).step_by(0x1000) {
            let page = self.create_page(addr + i as u64);
            page.set_addr((mem as u64 + i as u64) as u32);
            page.set_flag(flag);
        }
        println!("mmap: 0x{:x} -> 0x{:x}", addr, mem as usize);
        return addr as *mut u8;
    }

    pub fn get_phys(&self, addr: u64) -> *mut u8 {
        let page = self.get_page(addr);
        (page.get_addr() as u64 | addr & 0xfff) as *mut u8
    }

    #[allow(dead_code)]
    pub fn dump(&self) {
        println!("VirtualMemory:");
        println!("  root: 0x{:x}", self.root.addr);
        unsafe { dump(self.root.addr as *mut u64, 0) }
    }
}

#[allow(dead_code)]
unsafe fn dump(addr: *mut u64, level: u8) {
    println!("  addr: 0x{:x}", addr as u64);
    for i in 0..512 {
        let p = addr.add(i);
        if *p & 0b1 == 1 && level < 3 {
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

impl Clone for VirtualMemory {
    fn clone(&self) -> Self {
        VirtualMemory {
            root: self.root.clone(),
        }
    }
}
