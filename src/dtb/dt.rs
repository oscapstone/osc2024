use super::strings::StringMap;
use alloc::string::String;
use alloc::vec::Vec;
use stdio;

pub struct Dt {
    name: String,
    properties: Vec<Properties>,
    children: Vec<Dt>,
}

#[repr(u32)]
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
            _ => panic!("Invalid lexical value: {}", value),
        }
    }
}

// read untill null byte
fn read_string(addr: u32) -> String {
    // stdio::print("\t");
    // stdio::print("Reading string at: ");
    // stdio::print_u32(addr);
    // stdio::println("");
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
    // stdio::print("\t");
    // stdio::print("String: ");
    // stdio::print(&string);
    // stdio::println("");
    string
}

impl Dt {
    pub fn load(dt_addr: u32, strings: &StringMap) -> Dt {
        let mut dt = Dt {
            name: String::new(),
            properties: Vec::new(),
            children: Vec::new(),
        };
        let mut addr = dt_addr;
        stdio::print("DT address: ");
        stdio::print_u32(addr);
        stdio::println("");
        let mut layer = 0;
        loop {
            stdio::println("");
            // stdio::print("Reading lexical at: ");
            // stdio::print_u32(addr);
            // stdio::println("");

            let lexical = unsafe { core::ptr::read_volatile(addr as *const u32) };
            let lexical = lexical.swap_bytes();
            stdio::print("Lexical: ");
            stdio::print_u32(lexical);
            stdio::println("");
            let lexical = Lexical::from_u32(lexical);
            // let lexical = Lexical::from(lexical);
            addr += 4;
            match lexical {
                Lexical::BeginNode => {
                    layer += 1;
                    stdio::print("Begin node at: ");
                    stdio::print_u32(addr);
                    stdio::println("");
                    let name = read_string(addr);
                    dt.name = name.clone();
                    addr += name.len() as u32 + 1;
                    addr = (addr + 3) & !3;
                    // if name.len() == 0 {
                    //     addr += 4;
                    // }
                    stdio::print("Name: ");
                    stdio::print(&name);
                    // stdio::print(" size: ");
                    // stdio::print_u32(name.len() as u32);
                    stdio::println("");
                }
                Lexical::EndNode => {
                    if layer == 0 {
                        break;
                    }
                    layer -= 1;
                }
                Lexical::Prop => {
                    let properties = Properties::load(addr, strings);
                    dt.properties.push(properties.clone());
                    addr += properties.length + 8;
                    properties.print();
                    addr = (addr + 3) & !3;
                }
                Lexical::Nop => {
                    continue;
                }
                Lexical::End => {
                    break;
                }
            }
        }
        dt
    }

    pub fn print(&self) {
        stdio::print("Name: ");
        stdio::print(&self.name);
        stdio::println("");
        for property in self.properties.iter() {
            property.print();
        }
        for child in self.children.iter() {
            child.print();
        }
    }
}

#[derive(Clone)]
enum PropValue {
    integer(u32),
    string(String),
}

#[derive(Clone)]
pub struct Properties {
    length: u32,
    name: String,
    value: PropValue,
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

    fn print(&self) {
        stdio::print("\t");
        stdio::print("Property header length: ");
        stdio::print_u32(self.length);
        stdio::println("");
        stdio::print("\t");
        stdio::print("Property header name offset: ");
        stdio::print_u32(self.nameoff);
        stdio::println("");
    }
}

impl Properties {
    pub fn load(properties_addr: u32, strings: &StringMap) -> Properties {
        // stdio::print("\t");
        // stdio::print("Properties address: ");
        // stdio::print_u32(properties_addr);
        // stdio::println("");
        let header = PropertyHeader::load(properties_addr);
        // header.print();
        let name = strings.get(header.nameoff);
        let value = match header.length {
            4 => {
                let value =
                    unsafe { core::ptr::read_volatile((properties_addr + 8) as *const u32) }
                        .swap_bytes();
                PropValue::integer(value)
            }
            _ => {
                let value = read_string(properties_addr + 8);
                PropValue::string(value)
            }
        };
        Properties {
            length: header.length,
            name,
            value,
        }
    }
    pub fn print(&self) {
        stdio::print("Name: ");
        stdio::print(&self.name);
        stdio::println("");
        // stdio::print("Length: ");
        // stdio::print_u32(self.length);
        // stdio::println("");
        match self.value {
            PropValue::integer(value) => {
                stdio::print("Integer value: ");
                stdio::print_u32(value);
                stdio::println("");
            }
            PropValue::string(ref value) => {
                stdio::print("String value: ");
                stdio::print(value);
                stdio::println("");
            }
        }
    }
}
