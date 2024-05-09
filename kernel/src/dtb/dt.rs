use super::strings::StringMap;
use super::utils::read_string;
use alloc::string::String;
use alloc::vec::Vec;

#[derive(Debug)]
pub struct Dt {
    name: String,
    properties: Vec<Property>,
    children: Vec<Dt>,
    length: u32,
}

impl Dt {
    pub fn load(dt_addr: u32, strings: &StringMap) -> Dt {
        let mut dt = Dt {
            name: String::new(),
            properties: Vec::new(),
            children: Vec::new(),
            length: 0,
        };

        let mut addr = dt_addr;
        let lexical = unsafe { core::ptr::read_volatile(addr as *const u32) };

        assert_eq!(lexical.swap_bytes(), Lexical::BeginNode as u32);

        addr += 4;
        dt.name = read_string(addr);
        addr += dt.name.len() as u32 + 1;
        addr = (addr + 3) & !3;

        loop {
            let lexical = unsafe { core::ptr::read_volatile(addr as *const u32) };
            let lexical = lexical.swap_bytes();
            let lexical = Lexical::from_u32(lexical);
            match lexical {
                Lexical::BeginNode => {
                    let child = Dt::load(addr, strings);
                    addr += child.length;
                    dt.children.push(child);
                }
                Lexical::EndNode => {
                    addr += 4;
                    break;
                }
                Lexical::Prop => {
                    addr += 4;
                    let properties = Property::load(addr, strings);
                    addr += properties.length + 8;
                    dt.properties.push(properties);
                    addr = (addr + 3) & !3;
                }
                Lexical::Nop => {
                    addr += 4;
                    continue;
                }
                Lexical::End => {
                    break;
                }
            }
        }
        dt.length = addr - dt_addr;
        dt
    }
    pub fn get(&self, name: &str) -> Option<&Property> {
        if let Some(prop) = self.properties.iter().find(|prop| prop.name == name) {
            return Some(prop);
        } else {
            for child in self.children.iter() {
                if let Some(prop) = child.get(name) {
                    return Some(prop);
                }
            }
        }
        None
    }
}

#[derive(Debug)]
enum Lexical {
    BeginNode = 0x1,
    EndNode = 0x2,
    Prop = 0x3,
    Nop = 0x4,
    End = 0x9,
}

impl Lexical {
    fn from_u32(value: u32) -> Lexical {
        match value {
            0x1 => Lexical::BeginNode,
            0x2 => Lexical::EndNode,
            0x3 => Lexical::Prop,
            0x4 => Lexical::Nop,
            0x9 => Lexical::End,
            _ => panic!("Invalid lexical value {}", value),
        }
    }
}

#[derive(Debug)]
#[allow(dead_code)]
pub enum PropValue {
    Integer(u32),
    String(String),
}

struct PropertyHeader {
    length: u32,
    nameoff: u32,
}

impl PropertyHeader {
    fn load(addr: u32) -> PropertyHeader {
        let header = unsafe { &*(addr as *const PropertyHeader) };
        PropertyHeader {
            length: header.length.swap_bytes(),
            nameoff: header.nameoff.swap_bytes(),
        }
    }
}

#[derive(Debug)]
pub struct Property {
    length: u32,
    pub name: String,
    pub value: PropValue,
}

impl Property {
    fn load(property_addr: u32, strings: &StringMap) -> Property {
        let header = PropertyHeader::load(property_addr);
        let name = strings.get(header.nameoff);
        let value = match header.length {
            4 | 8 => {
                let value = unsafe { core::ptr::read_volatile((property_addr + 8) as *const u32) }
                    .swap_bytes();
                PropValue::Integer(value)
            }
            _ => {
                let value = read_string(property_addr + 8);
                PropValue::String(value)
            }
        };
        Property {
            length: header.length,
            name,
            value,
        }
    }
}
