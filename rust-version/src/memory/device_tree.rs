use core::{char, mem::size_of};

use crate::{print, println};
use alloc::{string::String, vec::Vec};

#[repr(u32)]
enum FdtToken {
    BeginNode = 0x1,
    EndNode = 0x2,
    Prop = 0x3,
    Nop = 0x4,
    End = 0x9,
}

#[repr(C)]
struct FdtHeader {
    magic: u32,
    totalsize: u32,
    off_dt_struct: u32,
    off_dt_strings: u32,
    off_mem_rsvmap: u32,
    version: u32,
    last_comp_version: u32,
    boot_cpuid_phys: u32,
    size_dt_strings: u32,
    size_dt_struct: u32,
}

#[repr(C)]
struct FdtProp {
    len: u32,
    nameoff: u32,
}

extern "C" {
    static mut __dtb: u64;
}

// align the pointer to n bytes
fn align_ptr(ptr: *const u8, n: usize) -> *const u8 {
    let offset = n - (ptr as usize % n);
    // println!("addr: {:x}, offset: {:x}", ptr as u64, offset as u64);
    if offset == n {
        ptr
    } else {
        unsafe { ptr.byte_offset(offset as isize) as *const u8 }
    }
}

// get the FdtHeader struct from the device tree pointer
// move the pointer to the start of the device tree struct
fn parse_fdt_header(mut ptr: *mut u8) -> FdtHeader {
    let header = FdtHeader {
        magic: unsafe { *(ptr as *const u32) }.swap_bytes(),
        totalsize: unsafe { *(ptr.offset(4) as *const u32) }.swap_bytes(),
        off_dt_struct: unsafe { *(ptr.offset(8) as *const u32) }.swap_bytes(),
        off_dt_strings: unsafe { *(ptr.offset(12) as *const u32) }.swap_bytes(),
        off_mem_rsvmap: unsafe { *(ptr.offset(16) as *const u32) }.swap_bytes(),
        version: unsafe { *(ptr.offset(20) as *const u32) }.swap_bytes(),
        last_comp_version: unsafe { *(ptr.offset(24) as *const u32) }.swap_bytes(),
        boot_cpuid_phys: unsafe { *(ptr.offset(28) as *const u32) }.swap_bytes(),
        size_dt_strings: unsafe { *(ptr.offset(32) as *const u32) }.swap_bytes(),
        size_dt_struct: unsafe { *(ptr.offset(36) as *const u32) }.swap_bytes(),
    };
    unsafe { ptr = ptr.offset(size_of::<FdtHeader>() as isize) as *mut u8 }
    header
}

fn get_nullterm_string(ptr: *const u8) -> String {
    let mut s = String::from("");
    let mut i = 0;
    loop {
        let c = unsafe { *ptr.offset(i) } as u8 as char;
        i += 1;
        s.push(c);
        if c == '\0' {
            break;
        }
    }
    s
}

impl TryFrom<u32> for FdtToken {
    type Error = &'static str;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0x1 => Ok(FdtToken::BeginNode),
            0x2 => Ok(FdtToken::EndNode),
            0x3 => Ok(FdtToken::Prop),
            0x4 => Ok(FdtToken::Nop),
            0x9 => Ok(FdtToken::End),
            _ => Err("Invalid Token"),
        }
    }
}

struct FdtManager {
    initlized: bool,
    fdt_header: FdtHeader,
    fdt_prop_cb: Vec<fn()>,
}

pub fn get_device_tree_ptr() -> *const u8 {
    let dev_tree_ptr_ptr = unsafe { __dtb as *const u64 };
    let dev_tree_ptr = unsafe { *dev_tree_ptr_ptr as *const u32 };
    println!(
        "Device Tree Pointer Pointer : {:x}",
        dev_tree_ptr_ptr as u64
    );
    println!("Device Tree Pointer : {:x}", dev_tree_ptr as u64);
    println!("Device Tree Pointer Value : {:x}", unsafe {
        *dev_tree_ptr as u64
    });

    dev_tree_ptr as *const u8
}

pub fn fdt_traverse() {
    let dev_tree_ptr = get_device_tree_ptr();

    // init FdtHeader struct with big endian
    let fdt_header = parse_fdt_header(dev_tree_ptr as *mut u8);
    let dev_tree_ptr = dev_tree_ptr as *const u8;

    // get the struct pointer from fdt_header
    let fdt_struct_ptr =
        unsafe { dev_tree_ptr.byte_offset(fdt_header.off_dt_struct as isize) as *const u32 };
    let fdt_strings_ptr =
        unsafe { dev_tree_ptr.byte_offset(fdt_header.off_dt_strings as isize) as *const u8 };

    let mut fdt_start = fdt_struct_ptr as *const u8;
    let fdt_end = unsafe { dev_tree_ptr.byte_offset(fdt_header.totalsize as isize) as *const u8 };
    // print all
    // println!("FDT Header : {:x}", fdt_header.magic);
    // println!("magic: {:x}, totalsize: {:x}, off_dt_struct: {:x}, off_dt_strings: {:x}, off_mem_rsvmap: {:x}, version: {:x}, last_comp_version: {:x}, boot_cpuid_phys: {:x}, size_dt_strings: {:x}, size_dt_struct: {:x}", fdt_header.magic, fdt_header.totalsize, fdt_header.off_dt_struct, fdt_header.off_dt_strings, fdt_header.off_mem_rsvmap, fdt_header.version, fdt_header.last_comp_version, fdt_header.boot_cpuid_phys, fdt_header.size_dt_strings, fdt_header.size_dt_struct);
    // println!("FDT Strings Pointer : {:x}", fdt_strings_ptr as u64);
    // println!("FDT Struct Pointer : {:x}", fdt_struct_ptr as u64);

    unsafe {
        while fdt_start < fdt_end {
            // evaluate the fdt_str_ptr to u32
            let fdt_struct_ptr = fdt_start as *const u32;
            let token = FdtToken::try_from((*fdt_struct_ptr).swap_bytes()).unwrap();
            fdt_start = fdt_struct_ptr.offset(1) as *mut u8;

            match token {
                FdtToken::BeginNode => {
                    // convert null terminated string to rust string
                    let node_name = get_nullterm_string(fdt_start as *const u8);
                    fdt_start =
                        align_ptr(fdt_start.byte_offset(node_name.len() as isize), 4) as *mut u8;
                    // println!("Begin Node, Name : {} {:x}", node_name, fdt_start as u64);
                }
                FdtToken::Prop => {
                    let mut fdt_prop = FdtProp {
                        len: *(fdt_start as *const u32),
                        nameoff: *(fdt_start.offset(4) as *const u32),
                    };

                    fdt_start = fdt_start.byte_offset(8);

                    fdt_prop.len = fdt_prop.len.swap_bytes();
                    fdt_prop.nameoff = fdt_prop.nameoff.swap_bytes();

                    let mut value = String::new();
                    for i in 0..fdt_prop.len {
                        value.push(*(fdt_start.offset(i as isize)) as char);
                    }

                    let key =
                        get_nullterm_string(fdt_strings_ptr.offset(fdt_prop.nameoff as isize));

                    if key.contains("linux,initrd-start") {
                        println!(
                            "kernel initrd start pos: {}",
                            (*(fdt_start as *const u32)).swap_bytes()
                        );
                    }

                    println!("Prop Name : {}, Value : {}", key, value);
                    fdt_start =
                        align_ptr(fdt_start.byte_offset(fdt_prop.len as isize) as *mut u8, 4)
                            as *mut u8;
                }

                FdtToken::EndNode => {
                    // println!("End Node");
                }

                FdtToken::Nop => {
                    // println!("NOP");
                }

                FdtToken::End => {
                    // println!("End");
                    break;
                }
            }
        }
    }
}
