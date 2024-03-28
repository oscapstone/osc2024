use super::strings::StringMap;
use alloc::string::String;
use alloc::vec::Vec;
use stdio::{print, print_u32, println};

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
        if lexical.swap_bytes() != Lexical::BeginNode as u32 {
            println("Invalid lexical value, should be begin ");
        }

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
            _ => panic!("Invalid lexical value"),
        }
    }
}

// read untill null byte
fn read_string(addr: u32) -> String {
    let mut addr = addr;
    let mut string = String::new();
    loop {
        let c = unsafe { core::ptr::read_volatile(addr as *const u8) };
        if c == 0 {
            break;
        }
        string.push(c as char);
        addr += 1;
    }
    string
}

enum PropValue {
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

pub struct Property {
    length: u32,
    name: String,
    value: PropValue,
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

#[allow(dead_code)]
impl Dt {
    pub fn print(&self) {
        self.print_with_indent(0);
    }

    fn print_with_indent(&self, indent: u32) {
        for _ in 0..indent {
            print("  ");
        }
        println(self.name.as_str());
        for prop in self.properties.iter() {
            for _ in 0..indent {
                print("  ");
            }
            print("  ");
            print(prop.name.as_str());
            print(": ");
            match &prop.value {
                PropValue::Integer(value) => {
                    print("0x");
                    print_u32(*value);
                    println("");
                }
                PropValue::String(value) => {
                    print(value.as_str());
                    println("");
                }
            }
        }
        for child in self.children.iter() {
            child.print_with_indent(indent + 1);
        }
    }
}

impl Property {
    pub fn print(&self) {
        print(self.name.as_str());
        print(": ");
        match &self.value {
            PropValue::Integer(value) => {
                print("0x");
                print_u32(*value);
                println("");
            }
            PropValue::String(value) => {
                print(value.as_str());
                println("");
            }
        }
    }
}
