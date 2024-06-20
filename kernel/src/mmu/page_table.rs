use super::config::ENTRY_COUNT;
use super::entry::Entry;
use alloc::alloc::alloc;
use alloc::boxed::Box;
use alloc::vec;
use alloc::vec::Vec;
use core::alloc::Layout;

use crate::mmu::config::MAIR_NORMAL_NC_IDX;
use crate::mmu::config::PD_ACCESS;
use crate::mmu::config::PD_PAGE;
use crate::mmu::config::PD_TABLE;

#[derive(Debug)]
pub struct PageTable {
    entries: Vec<Entry>,
    pub addr: u64,
}

impl PageTable {
    pub fn new() -> Self {
        let addr = unsafe { alloc(Layout::from_size_align(0x1000, 0x1000).unwrap()) as u64 };
        unsafe {
            let mut p = addr as *mut u8;
            for _ in 0..0x1000 {
                *p = 0;
                p = p.add(1);
            }
        }
        PageTable {
            entries: vec![Entry::new(); ENTRY_COUNT],
            addr,
        }
    }

    pub fn load(addr: u64, level: u64) -> Self {
        // println!("PageTable::load: 0x{:x}", addr);
        let mut entries = vec![Entry::new(); ENTRY_COUNT];
        for i in 0..ENTRY_COUNT {
            let entry_addr = addr + (i * 8) as u64;
            let entry = unsafe { *(entry_addr as *const u64) };
            if entry & 0b1 == 1 {
                if level < 3 {
                    let pt = Box::new(PageTable::load(entry & 0xffff_ffff_ffff_f000, level + 1));
                    entries[i] = Entry::PdTable(pt);
                } else {
                    entries[i] = Entry::PdBlock((entry_addr as *mut u64, entry));
                }
            }
        }
        PageTable { entries, addr }
    }

    pub fn get_entry(&self, idx: usize) -> &Entry {
        &self.entries[idx]
    }

    pub fn get_page(&self, addr: u64, level: u64) -> &Entry {
        let idx = ((addr >> (12 + 9 * (3 - level))) & 0x1ff) as usize;
        if level == 3 {
            return self.get_entry(idx);
        }
        if !self.exists(idx) {
            return &Entry::None;
        }
        if let Entry::PdTable(pt) = self.get_entry(idx) {
            return pt.get_page(addr, level + 1);
        }
        &Entry::None
    }

    pub fn create_page(&mut self, addr: u64, level: u64) -> &mut Entry {
        let idx = ((addr >> (12 + 9 * (3 - level))) & 0x1ff) as usize;
        if level == 3 {
            if !self.exists(idx as usize) {
                self.set_entry(
                    idx as usize,
                    Entry::PdBlock(((self.addr + (idx * 8) as u64) as *mut u64, 0)),
                );
            }
            return self.get_entry_mut(idx as usize);
        }
        if !self.exists(idx as usize) {
            let pg = PageTable::new();
            self.set_entry(idx as usize, Entry::PdTable(Box::new(pg)));
            if let Entry::PdTable(pt) = self.get_entry_mut(idx as usize) {
                return pt.create_page(addr, level + 1);
            }
            panic!("get_page: not a PdTable");
        }
        let entry = self.get_entry_mut(idx as usize);
        if let Entry::PdTable(pt) = entry {
            return pt.create_page(addr, level + 1);
        }
        panic!("get_page: not a PdTable");
    }

    pub fn get_entry_mut(&mut self, idx: usize) -> &mut Entry {
        &mut self.entries[idx]
    }

    pub fn set_entry(&mut self, idx: usize, entry: Entry) {
        assert!(idx < ENTRY_COUNT);
        assert!(self.entries[idx].is_valid() == false);
        self.entries[idx] = entry;
        match &self.entries[idx] {
            Entry::PdTable(pt) => {
                let entry_addr = self.addr + (idx * 8) as u64;
                unsafe {
                    let entry = entry_addr as *mut u64;
                    *entry =
                        pt.addr | (PD_TABLE | PD_ACCESS | (MAIR_NORMAL_NC_IDX << 2) as u32) as u64;
                }
            }
            Entry::PdBlock((saddr, addr)) => {
                assert!(*addr == 0);
                unsafe {
                    let entry = *saddr as *mut u64;
                    *entry = addr | (PD_PAGE | PD_ACCESS | (MAIR_NORMAL_NC_IDX << 2) as u32) as u64;
                }
            }
            _ => {}
        }
    }

    pub fn exists(&self, idx: usize) -> bool {
        self.entries[idx].is_valid()
    }
}

impl Clone for PageTable {
    fn clone(&self) -> Self {
        let addr = unsafe { alloc(Layout::from_size_align(0x1000, 0x1000).unwrap()) as u64 };
        unsafe {
            let mut p = addr as *mut u8;
            for _ in 0..0x1000 {
                *p = 0;
                p = p.add(1);
            }
        };
        let entries = self.entries.clone();
        for i in 0..ENTRY_COUNT {
            if self.entries[i].is_valid() {
                let entry = self.entries[i].clone();
                match entry {
                    Entry::PdBlock((saddr, pg)) => {
                        assert!(saddr as u64 == 0);
                        let entry_addr = addr + (i * 8) as u64;
                        unsafe {
                            let entry = entry_addr as *mut u64;
                            *entry = pg;
                        }
                    }
                    Entry::PdTable(pt) => {
                        let pt = pt.clone();
                        let entry_addr = addr + (i * 8) as u64;
                        unsafe {
                            let entry = entry_addr as *mut u64;
                            *entry = pt.addr
                                | (PD_TABLE | PD_ACCESS | (MAIR_NORMAL_NC_IDX << 2) as u32) as u64;
                        }
                    }
                    _ => {}
                }
            }
        }
        PageTable { entries, addr }
    }
}
