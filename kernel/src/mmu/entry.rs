use core::fmt::Debug;

use super::page_table::PageTable;
use alloc::boxed::Box;

#[derive(Clone)]
pub enum Entry {
    None,
    PdBlock((*mut u64, u64)),
    PdTable(Box<PageTable>),
}

impl Entry {
    pub fn new() -> Self {
        Entry::None
    }

    pub fn is_valid(&self) -> bool {
        match self {
            Entry::None => false,
            _ => true,
        }
    }

    pub fn set_addr(&mut self, addr: u32) {
        match self {
            Entry::PdBlock((saddr, pg)) => {
                *pg = (*pg & 0xfff) | (addr as u64);
                unsafe { **saddr = *pg }
            }
            _ => panic!("set_addr: not a PdBlock"),
        }
    }

    pub fn set_flag(&mut self, flag: u32) {
        match self {
            Entry::PdBlock((saddr, pg)) => {
                *pg = (*pg & !0xfff) | (flag as u64);
                unsafe { **saddr = *pg }
            }
            _ => panic!("set_flag: not a PdBlock"),
        }
    }

    pub fn get_addr(&self) -> *mut u8 {
        match self {
            Entry::PdBlock((_, pg)) => (*pg & 0xffff_ffff_f000) as *mut u8,
            _ => panic!("get_addr: not a PdBlock"),
        }
    }
}

impl Debug for Entry {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Entry::None => write!(f, "None"),
            Entry::PdBlock((_, pg)) => write!(f, "PdBlock(0x{:x})", pg),
            Entry::PdTable(tbl) => write!(f, "PdTable 0x{:x}", tbl.addr),
        }
    }
}
