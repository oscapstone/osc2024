use crate::println;

pub mod dt;
mod dt_header;
mod dt_prop;
mod dt_string;
mod dt_struct;

// use special memory address to pass x0 from bootloader to kernel
extern "C" {
    static __dtb_address: u32;
}

fn get_dtb_address() -> u32 {
    // let ptr_dtb_address = unsafe { __dtb_address as *const u32 };

    let dtb_address = unsafe { core::ptr::read_volatile(__dtb_address as *const u32) };

    // let dtb_address = unsafe { *ptr_dtb_address as *const u32 };

    // loop{
    //   println!("[Device Tree] Value of ptr_dtb_address: {:#x}", dtb_address as u32);
    // }
    // let ptr_dtb_magic = unsafe { *dtb_address as *const u32 };
    // let dtb_magic = unsafe { *ptr_dtb_magic };

    //println!("[Device Tree] Value of __dtb_address: {:#x}", unsafe {__dtb_address });
    //println!("[Device Tree] Value of ptr_dtb_address: {:#x}", ptr_dtb_address as u32);
    //println!("[Device Tree] Value of dtb_address: {:#x}", dtb_address as u32);
    //println!("[Device Tree] Value of dtb_magic: {:#x}", dtb_magic.swap_bytes());

    // checking dtb address has valid magic value
    // if dtb_magic.swap_bytes() != 0xd00dfeed {
    //   loop{
    //     println!("[Device Tree] invalid dtb address");
    //   }
    //   //println!("[Device Tree] invalid dtb address");
    // }

    // unsafe{*dtb_address}
    dtb_address as u32
}

pub fn load_dtb() -> dt::Dt {
    let dtb_address = get_dtb_address();
    // println!("[Device Tree] dtb_address: {:#x}", dtb_address);

    let fdt_header = dt_header::FdtHeader::load(dtb_address);

    if fdt_header.valid_magic() == false {
        println!("[Device Tree] fdtHeader magic non valid");
    }

    let strings_addr = dtb_address + fdt_header.get_off_dt_strings();
    //println!("[Device Tree header] off_dt_strings: {:#x}",strings_addr);
    let dt_strings = dt_string::DtString::load(strings_addr);

    let dt_struct_addr = dtb_address + fdt_header.get_off_dt_struct();
    //println!("[Device Tree] dt_struct_addr: {:#x}",dt_struct_addr);

    let dt = dt::Dt::load(dt_struct_addr, &dt_strings);

    dt
}

pub fn get_initrd_start() -> Option<u32> {
    let dt = load_dtb();
    let node = dt.get_prop("linux,initrd-start");
    match node {
        Some(node) => match node.value {
            dt_prop::PropValue::Integer(value) => Some(value),
            _ => None,
        },
        None => None,
    }
}
