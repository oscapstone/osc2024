extern crate alloc;

use super::dt_string::DtString;
use alloc::string::String;

#[derive(Debug)]
#[allow(dead_code)]
pub enum PropValue {
    Integer(u32),
    String(String),
}

struct PropHeader {
    length: u32,
    nameoff: u32,
}

impl PropHeader {
    fn load(addr: u32) -> PropHeader {
        let header = unsafe { &*(addr as *const PropHeader) };
        PropHeader {
            length: header.length.swap_bytes(),
            nameoff: header.nameoff.swap_bytes(),
        }
    }
}

#[derive(Debug)]
pub struct Property {
    pub length: u32,
    pub name: String,
    pub value: PropValue,
}

impl Property {
    pub fn load(property_addr: u32, strings: &DtString) -> Property {
        let header = PropHeader::load(property_addr);
        // println!(
        //     "[Device Tree Prop] Prop Header Loaded ,length: {}, nameof: {}",
        //     header.length,
        //     header.nameoff
        // );
        let name: String = strings.get(header.nameoff);
        //println!("[Device Tree Prop] String Loaded");
        let value = match header.length {
            4 | 8 => {
                //println!("[Device Tree Prop] Int property_addr: {:#x}", property_addr + 8);
                let value = unsafe { core::ptr::read_volatile((property_addr + 8) as *const u32) }
                    .swap_bytes();
                PropValue::Integer(value)
            }
            _ => {
                //println!("[Device Tree Prop] String property_addr: {:#x}", property_addr + 8);
                let value = load_data(property_addr + 8, header.length);
                PropValue::String(value)
            }
        };
        Property {
            length: header.length + 8,
            name,
            value,
        }
    }

    pub fn get_length(&self) -> u32 {
        self.length
    }
}

// get string end with '\0'
pub fn load_data(base_address: u32, len: u32) -> String {
    let mut addr = base_address;
    let mut string = String::new();
    for _counter in 0..len {
        let c = unsafe { core::ptr::read_volatile(addr as *const u8) };
        string.push(c as char);
        addr += 1;
    }
    //println!("[Device Tree load_data] String: {} Addr: {:#x}", string ,addr);
    string
}
