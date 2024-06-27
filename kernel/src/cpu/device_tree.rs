use crate::println;
use alloc::{
    string::{String, ToString},
    vec::Vec,
};

use crate::os::stdio::*;

#[allow(dead_code)]
enum FdtHeader {
    Magic = 0,
    TotalSize = 1,
    OffDtStruct = 2,
    OffDtStrings = 3,
    OffMemRsvMap = 4,
    Version = 5,
    LastCompVersion = 6,
    BootCpuidPhys = 7,
    SizeDtStrings = 8,
    SizeDtStruct = 9,
}

pub struct DeviceTree {
    device_tree_ptr: *const u8,
    total_size: u32,
    memrsv_ptr: *const u64,
    struct_ptr: *const u32,
    struct_size: u32,
    strings_ptr: *const u8,
    strings_size: u32,
    version: u32,
    tree_root: Node,
}

struct FdtReserveEntry {
    address: u64,
    size: u64,
}

struct FdtPropEntry {
    len: u32,
    nameoff: u32,
}

struct NodeProp {
    name: String,
    value: Vec<u8>,
}
struct Node {
    name: String,
    properties: Vec<NodeProp>,
    children: Vec<Node>,
}

impl Node {
    fn print(&self, depth: u32) {
        for _ in 0..depth {
            print("  ");
        }
        print(&self.name);
        println("");
        for prop in &self.properties {
            for _ in 0..depth {
                print("  ");
            }
            print(&prop.name);
            print(": ");
            for c in &prop.value {
                print((*c as char).to_string().as_str());
            }
            println("");
        }
        for child in &self.children {
            child.print(depth + 1);
        }
    }

    fn get(&self, name: &str) -> Option<&Vec<u8>> {
        for prop in &self.properties {
            if prop.name == name {
                return Some(&prop.value);
            }
        }
        for child in &self.children {
            let ret = child.get(name);
            if ret.is_some() {
                return ret;
            }
        }
        None
    }
}

impl Default for DeviceTree {
    fn default() -> Self {
        DeviceTree {
            device_tree_ptr: 0 as *const u8,
            total_size: 0,
            memrsv_ptr: 0 as *const u64,
            struct_ptr: 0 as *const u32,
            strings_ptr: 0 as *const u8,
            version: 0,
            struct_size: 0,
            strings_size: 0,
            tree_root: Node {
                name: String::from("root"),
                properties: Vec::new(),
                children: Vec::new(),
            },
        }
    }
}

impl DeviceTree {
    const DEVICE_TREE_ADDRESS: *const u64 = 0x75100 as *const u64;
    const FDT_BEGIN_NODE: u32 = 0x00000001;
    const FDT_END_NODE: u32 = 0x00000002;
    const FDT_PROP: u32 = 0x00000003;
    const FDT_NOP: u32 = 0x00000004;
    const FDT_END: u32 = 0x00000009;

    pub fn get(&self, name: &str) -> Option<&Vec<u8>> {
        self.tree_root.get(name)
    }

    pub fn get_address() -> *mut u8 {
        (unsafe { *(DeviceTree::DEVICE_TREE_ADDRESS) } | 0xFFFF_0000_0000_0000) as *mut u8
    }

    pub fn init() -> DeviceTree {
        let mut ret = DeviceTree {
            device_tree_ptr: DeviceTree::get_address(),
            ..Default::default()
        };

        // print("Device tree pointer: ");
        // print_hex(ret.device_tree_ptr as u32);
        // for i in 0..32 {
        //     print_hex(unsafe { *(ret.device_tree_ptr.offset(i as isize)) as u32});
        // }

        // print("Magic: ");
        // print_hex(ret.get_header(FdtHeader::Magic));

        ret.total_size = ret.get_header(FdtHeader::TotalSize);
        ret.struct_ptr = unsafe {
            ret.device_tree_ptr
                .offset(ret.get_header(FdtHeader::OffDtStruct) as isize) as *const u32
        };
        ret.strings_ptr = unsafe {
            ret.device_tree_ptr
                .offset(ret.get_header(FdtHeader::OffDtStrings) as isize) as *const u8
        };
        ret.memrsv_ptr = unsafe {
            ret.device_tree_ptr
                .offset(ret.get_header(FdtHeader::OffMemRsvMap) as isize) as *const u64
        };
        ret.version = ret.get_header(FdtHeader::Version);
        ret.struct_size = ret.get_header(FdtHeader::SizeDtStruct);
        ret.strings_size = ret.get_header(FdtHeader::SizeDtStrings);

        // print_hex(ret.strings_size);

        let mut memory_reservation_map: Vec<FdtReserveEntry> = Vec::new();
        let mut i = 0;
        // Parse memory reservation map
        // println("Memory reservation map:");
        loop {
            memory_reservation_map.push(FdtReserveEntry {
                address: unsafe { (*(ret.memrsv_ptr.offset(i))).swap_bytes() },
                size: unsafe { (*(ret.memrsv_ptr.offset(i + 1))).swap_bytes() },
            });
            if memory_reservation_map.last().unwrap().address == 0
                && memory_reservation_map.last().unwrap().size == 0
            {
                memory_reservation_map.pop();
                break;
            }
            i += 2;
        }

        // Parse device tree structure
        // println("Device tree structure:");
        let mut root = Node {
            name: String::from("root"),
            properties: Vec::new(),
            children: Vec::new(),
        };
        ret.construct_node(&mut root, 0, 0);

        ret.tree_root = root;

        ret
    }

    fn construct_node(&self, now_node: &mut Node, offset: usize, depth: u32) -> usize {
        // println_now("Constructing node");
        // print_hex_now(offset as u32);
        // print_hex_now(depth);
        let mut i = offset;
        loop {
            // unsafe {
            //     print_hex((self.struct_ptr.add(i)) as u32);
            // }
            match unsafe { (*(self.struct_ptr.add(i))).swap_bytes() } {
                DeviceTree::FDT_BEGIN_NODE => {
                    // println("Begin node");
                    i += 1;

                    let mut node = Node {
                        name: String::new(),
                        properties: Vec::new(),
                        children: Vec::new(),
                    };
                    // println("Node name: ");
                    'aa: loop {
                        let a = unsafe { *(self.struct_ptr).add(i) };
                        for k in 0..4 {
                            let b = (a >> (k * 8) & 0xFF) as u8;
                            node.name.push(b as char);
                            if b == 0 {
                                i += 1;
                                break 'aa;
                            }
                        }
                        i += 1;
                    }
                    let name = node.name.clone();

                    now_node.children.push(node);
                    // println("Pushed child node");
                    let len = now_node.children.len();
                    if now_node.children[len - 1].name != name {
                        println("Error");
                        loop {}
                    }
                    // println("Constructing child node");
                    i = self.construct_node(
                        &mut now_node.children.last_mut().unwrap(),
                        i,
                        depth + 1,
                    );
                }
                DeviceTree::FDT_PROP => {
                    i += 1;

                    let entry = FdtPropEntry {
                        len: unsafe { (*(self.struct_ptr).add(i)).swap_bytes() },
                        nameoff: unsafe { (*(self.struct_ptr).add(i + 1)).swap_bytes() },
                    };
                    i += 2;

                    let mut prop_node = NodeProp {
                        name: self.get_string(entry.nameoff),
                        value: Vec::new(),
                    };

                    if entry.len > 0 {
                        'aa: loop {
                            let a = unsafe { *(self.struct_ptr).add(i) };
                            for k in 0..4 {
                                let b = (a >> (k * 8) & 0xFF) as u8;
                                prop_node.value.push(b);
                                if prop_node.value.len() >= entry.len as usize {
                                    i += 1;
                                    break 'aa;
                                }
                            }
                            i += 1;
                        }
                    }

                    let name = prop_node.name.clone();

                    // println(&prop_node.name);

                    now_node.properties.push(prop_node);
                    let len = now_node.properties.len();
                    if now_node.properties[len - 1].name != name {
                        println("Error");
                        loop {}
                    }
                }
                DeviceTree::FDT_END_NODE => {
                    // println("End node");
                    i += 1;
                    return i;
                }
                DeviceTree::FDT_NOP => {
                    i += 1;
                }
                DeviceTree::FDT_END => {
                    break i;
                }
                unknown => {
                    println!("Unknown node type: 0x{:X}", unknown);
                    break i;
                }
            }
        }
    }

    fn get_string(&self, offset: u32) -> String {
        let str_ptr = unsafe { self.strings_ptr.offset(offset as isize) };
        let mut string = String::new();
        let mut i = 0;
        loop {
            let c = unsafe { *str_ptr.offset(i) };
            if c == 0 {
                break;
            }
            string.push(c as char);
            i += 1;
        }
        string
    }

    fn get_header(&self, header: FdtHeader) -> u32 {
        unsafe { (*(self.device_tree_ptr.offset(header as isize * 4) as *const u32)).swap_bytes() }
    }
}
