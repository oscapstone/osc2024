extern crate alloc;

use super::dt_prop::Property;
use super::dt_string::load_string;
use super::dt_string::DtString;
use alloc::string::String;
use alloc::vec::Vec;

#[derive(Debug)]
pub struct Dt {
    name: String,
    properties: Vec<Property>,
    children: Vec<Dt>,
    length: u32,
}

#[allow(unused_assignments)]
impl Dt {
    pub fn load(dt_struct_addr: u32, dt_string: &DtString) -> Dt {
        let mut dt = Dt {
            name: String::new(),
            properties: Vec::new(),
            children: Vec::new(),
            length: 0,
        };

        //println!("[Device Tree Dt Root] dt_struct_addr: {:#x}", dt_struct_addr);

        let dt_struct_begin = unsafe { core::ptr::read_volatile(dt_struct_addr as *const u32) };
        // let dt_struct_begin = unsafe { &*(dt_struct_addr as *const u32) };
        // let dt_struct_begin = dt_struct_addr;

        // check fdt struct start with begin
        if dt_struct_begin.swap_bytes() != FdtTag::FdtBeginNode as u32 {
            //println!("[Device Tree Dt Root] FdtBeginNode Error, value {:#x}, expected: {:#x}", dt_struct_begin, MAGIC_NUMBER);
        } else {
            //println!("[Device Tree Dt Root] FdtBeginNode OK!");
        }
        let struct_root_addr: u32 = dt_struct_addr + 4;

        // create root Device tree node
        dt.name = load_string(struct_root_addr);
        //println!("[Device Tree Dt Root] struct_root_addr: {:#x}, Root name: {}",struct_root_addr , dt.name);

        // get the next tag addr
        // add nullterm
        let mut index_addr = struct_root_addr + dt.name.len() as u32 + 1;
        index_addr = align_up(index_addr,4);
        // let mut index_addr = align_addr(struct_root_addr + dt.name.len() as u32 + 1, 4) + 4;
        //println!("[Device Tree Dt Root] index_addr: {:#x}", index_addr);

        // let mut counter:u32 =  0;

        loop {
            let tag = unsafe { core::ptr::read_volatile(index_addr as *const u32) };
            let tag = tag.swap_bytes();

            //println!("[Device Tree Dt] index_addr: {:#x} FdtTag: {:#x} counter: {}", index_addr, tag, counter);
            // counter+=1;

            let tag = FdtTag::try_from(tag).unwrap();

            match tag {
                // 0x1
                FdtTag::FdtBeginNode => {
                    //println!("[Device Tree Dt FdtBeginNode]");
                    let child = Dt::load(index_addr, dt_string);
                    index_addr += child.length;
                    dt.children.push(child);
                }
                // 0x2
                FdtTag::FdtEndNode => {
                    //println!("[Device Tree Dt FdtEndNode]");
                    index_addr += 4;
                    break;
                }
                // 0x3
                FdtTag::FdtProp => {
                    index_addr += 4;
                    //println!("[Device Tree Dt FdtProp] index_addr: {:#x}",index_addr);
                    let property = Property::load(index_addr, dt_string);
                    index_addr += property.get_length();
                    // index_addr += 8;
                    //println!("[Device Tree Dt FdtProp] property name: {}, propLen: {:#x}, index_addr: {:#x}",property.name, property.get_length(), index_addr);
                    dt.properties.push(property);
                    index_addr = align_up(index_addr,4);
                }
                // 0x4
                FdtTag::FdtNop => {
                    //println!("[Device Tree Dt FdtNop]");
                    index_addr += 4;
                    continue;
                }
                // 0x5
                FdtTag::FdtEnd => {
                    //println!("[Device Tree Dt FdtEnd]");
                    // index_addr;
                    break;
                }
            }
        }

        dt.length = index_addr - dt_struct_addr;

        // return
        dt
    }

    pub fn get_prop(&self, name: &str) -> Option<&Property> {
        if let Some(prop) = self.properties.iter().find(|prop| prop.name == name) {
            return Some(prop);
        } else {
            for child in self.children.iter() {
                if let Some(prop) = child.get_prop(name) {
                    return Some(prop);
                }
            }
        }
        None
    }
}

#[repr(u32)]
pub enum FdtTag {
    FdtBeginNode = 0x1, // start name full name
    FdtEndNode = 0x2,   // end
    FdtProp = 0x3,
    FdtNop = 0x4,
    FdtEnd = 0x9,
}

impl TryFrom<u32> for FdtTag {
    type Error = &'static str;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0x1 => Ok(FdtTag::FdtBeginNode),
            0x2 => Ok(FdtTag::FdtEndNode),
            0x3 => Ok(FdtTag::FdtProp),
            0x4 => Ok(FdtTag::FdtNop),
            0x9 => Ok(FdtTag::FdtEnd),
            _ => Err("Invalid Token"),
        }
    }
}


fn align_up(addr: u32, align: u32) -> u32 {
    match addr % align {
        0 => addr,
        remainder => addr + align - remainder,
    }
}


// align the addr to n bytes
// fn align_addr(addr: u32, n: u32) -> u32 {
//     // (index_addr + 3) & !3;
//     let offset = n - (addr % n);
//     if offset == n {
//         addr
//     } else {
//         addr + offset
//     }
// }
