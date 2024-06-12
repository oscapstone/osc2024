use super::config::ENTRY_COUNT;
use super::entry::Entry;
use alloc::alloc::alloc;
use alloc::boxed::Box;
use alloc::vec;
use alloc::vec::Vec;
use core::alloc::Layout;

use crate::mmu::MAIR_NORMAL_NC_IDX;
use crate::mmu::PD_ACCESS;
use crate::mmu::PD_PAGE;
use crate::mmu::PD_TABLE;

#[derive(Debug, Clone)]
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

    pub fn get_page(&mut self, addr: u64, level: u64) -> &mut Entry {
        let idx = ((addr >> (12 + 9 * (3 - level))) & 0x1ff) as usize;
        if level == 3 {
            if !self.exists(idx as usize) {
                self.set_entry(
                    idx as usize,
                    Entry::PdBlock(((self.addr + (idx * 8) as u64) as *mut u64, 0)),
                );
            }
            return self.get_entry(idx as usize);
        }
        if !self.exists(idx as usize) {
            let pg = PageTable::new();
            self.set_entry(idx as usize, Entry::PdTable(Box::new(pg)));
            if let Entry::PdTable(pt) = self.get_entry(idx as usize) {
                return pt.get_page(addr, level + 1);
            }
            panic!("get_page: not a PdTable");
        }
        let entry = self.get_entry(idx as usize);
        if let Entry::PdTable(pt) = entry {
            return pt.get_page(addr, level + 1);
        }
        panic!("get_page: not a PdTable");
    }

    pub fn get_entry(&mut self, idx: usize) -> &mut Entry {
        &mut self.entries[idx]
    }

    pub fn set_entry(&mut self, idx: usize, entry: Entry) {
        assert!(idx < ENTRY_COUNT);
        assert!(self.entries[idx].is_valid() == false);
        self.entries[idx] = entry.clone();
        match entry {
            Entry::PdTable(pt) => {
                let entry_addr = self.addr + (idx * 8) as u64;
                unsafe {
                    let entry = entry_addr as *mut u64;
                    *entry =
                        pt.addr | (PD_TABLE | PD_ACCESS | (MAIR_NORMAL_NC_IDX << 2) as u32) as u64;
                }
            }
            Entry::PdBlock((saddr, addr)) => {
                assert!(addr == 0);
                unsafe {
                    let entry = saddr as *mut u64;
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
