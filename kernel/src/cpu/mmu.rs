use alloc::vec::Vec;
use core::alloc::Layout;
use core::arch::asm;
use core::ptr::{self, null_mut, read_volatile, write_volatile};

use crate::os::stdio::{print_hex_now, println_now};
use crate::println;

const TCR_CONFIG_REGION_48BIT: usize = ((64 - 48) << 0) | ((64 - 48) << 16);
const TCR_CONFIG_4KB: usize = (0b00 << 14) | (0b00 << 30);
const TCR_CONFIG_DEFAULT: usize = TCR_CONFIG_REGION_48BIT | TCR_CONFIG_4KB | (0b101usize << 32);

const MAIR_DEVICE_NG_NR_NE: usize = 0b00000000;
const MAIR_NORMAL_NOCACHE: usize = 0b01000100;
const MAIR_IDX_DEVICE_NG_NR_NE: usize = 0;
const MAIR_IDX_NORMAL_NOCACHE: usize = 1;
const MAIR_CONFIG_DEFAULT: usize = (MAIR_DEVICE_NG_NR_NE << (MAIR_IDX_DEVICE_NG_NR_NE * 8))
    | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8));

const PD_TABLE: usize = 0b11;
const PD_BLOCK: usize = 0b01;
const PD_ACCESS: usize = 1 << 10;

const AP_RW_EL0: usize = 0b01 << 6;
const AP_RO_EL0: usize = 0b11 << 6;

const BOOT_PGD_ATTR: usize = PD_TABLE;
const BOOT_PUD_ATTR_DEVICE: usize =
    PD_ACCESS | (0 << 6) | (MAIR_IDX_DEVICE_NG_NR_NE << 2) | PD_BLOCK;
const BOOT_PUD_ATTR_NORMAL: usize =
    PD_ACCESS | (0 << 6) | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK;

const PGD_ADDRESS: usize = 0x2000;
const PUD_ADDRESS: usize = 0x3000;
const PMD_ADDRESS: usize = 0x4000;

#[no_mangle]
unsafe extern "C" fn setup_mmu() {
    // Initialize TCR
    asm!(
        "msr tcr_el1, {0}",
        in(reg) TCR_CONFIG_DEFAULT,
    );

    // Initialize MAIR
    asm!(
        "msr mair_el1, {0}",
        in(reg) MAIR_CONFIG_DEFAULT,
    );

    // Identity Paging
    write_volatile(PGD_ADDRESS as *mut usize, PUD_ADDRESS | BOOT_PGD_ATTR);
    write_volatile(
        PUD_ADDRESS as *mut usize,
        PMD_ADDRESS | PD_ACCESS | AP_RW_EL0 | MAIR_IDX_NORMAL_NOCACHE << 2 | PD_TABLE,
    );
    write_volatile(
        (PUD_ADDRESS + 8) as *mut usize,
        0x40000000usize | BOOT_PUD_ATTR_DEVICE,
    );

    const PMD_SIZE: usize = 1024 * 1024 * 2;

    for i in 0..512 {
        let entry_address = (PMD_ADDRESS + i * 8) as *mut usize;
        let page_address = i * PMD_SIZE;

        if page_address < 0x1000 {
            write_volatile(
                entry_address,
                page_address | PD_ACCESS | MAIR_IDX_NORMAL_NOCACHE << 2 | PD_BLOCK,
            );
        } else if page_address < 0x3C00_0000 {
            write_volatile(
                entry_address,
                page_address | PD_ACCESS | AP_RW_EL0 | MAIR_IDX_NORMAL_NOCACHE << 2 | PD_BLOCK,
            );
        } else {
            write_volatile(
                entry_address,
                page_address | PD_ACCESS | (MAIR_IDX_DEVICE_NG_NR_NE << 2 | PD_BLOCK),
            );
        }
    }

    asm!(
        "msr ttbr0_el1, {PGD_addr}",
        "msr ttbr1_el1, {PGD_addr}",

        PGD_addr = in(reg) PGD_ADDRESS,
    );

    // Enable MMU
    asm!(
        "mrs {tmp}, sctlr_el1",
        "orr {tmp}, {tmp}, 1",
        "msr sctlr_el1, {tmp}",
        tmp = out(reg) _,
    );
}

pub fn vm_setup(
    program_ptr: *mut u8,
    program_size: usize,
    stack_ptr: *mut u8,
    stack_size: usize,
) -> *mut u8 {
    let pgd_page_ptr = unsafe {
        alloc::alloc::alloc(Layout::from_size_align(4096, 4096).expect("Cannot allocate"))
    };

    unsafe {
        core::ptr::write_bytes(pgd_page_ptr, 0, 4096);
    }

    // println!("=====PROGRAM=====");
    // program
    vm_setup_recursive(
        0,
        pgd_page_ptr as *mut usize,
        0,
        0,
        program_ptr,
        program_size,
        MAIR_NORMAL_NOCACHE,
    );

    // println!("=====STACK=====");
    // stack
    vm_setup_recursive(
        0,
        pgd_page_ptr as *mut usize,
        0,
        0x1_0000_0000_0000 - stack_size,
        stack_ptr,
        stack_size,
        MAIR_NORMAL_NOCACHE,
    );

    // GPU
    vm_setup_recursive(
        0,
        pgd_page_ptr as *mut usize,
        0,
        0x3C00_0000,
        0x3C00_0000 as *mut u8,
        0x400_0000,
        MAIR_DEVICE_NG_NR_NE,
    );

    pgd_page_ptr.mask(0xFFFF_FFFF)
}

fn vm_setup_recursive(
    level: u8,
    page_table_ptr: *mut usize,
    page_table_base: usize,
    vm_addr: usize,
    pm_addr: *mut u8,
    size: usize,
    MAIR: usize,
) {
    if level > 3 {
        return;
    }

    let memory_size_per_entry: usize = match level {
        0 => 1024 * 1024 * 1024 * 512,
        1 => 1024 * 1024 * 1024,
        2 => 1024 * 1024 * 2,
        3 => 1024 * 4,
        _ => panic!("Cannot find level"),
    };

    // if level < 4 {
    //     println!("\nLevel: {}", level);
    //     println!("Page table addr: {:x}", page_table_ptr as usize);
    //     println!("Page table base: {:x}", page_table_base);
    //     println!("vm_addr: {:x}", vm_addr);
    //     println!("entry_size: {} byte", memory_size_per_entry);
    //     println!("size: {} byte", size);
    // }

    assert!(
        memory_size_per_entry * 512 >= size,
        "Cannot fit in virtual memory table"
    );

    for i in 0..512 {
        let entry_ptr = unsafe { page_table_ptr.add(i) };
        let entry_vm_addr = page_table_base + i * memory_size_per_entry;

        if vm_addr + size <= entry_vm_addr || entry_vm_addr + memory_size_per_entry <= vm_addr {
            // unsafe {
            //     write_volatile(entry_ptr, 0);
            // }
            continue;
        }

        // println!("entry_ptr: {:x}", entry_ptr as usize);
        // println!("entry_vm_addr: {:x}", entry_vm_addr);

        let address;
        let entry;
        let original_entry = unsafe { read_volatile(entry_ptr) };

        if level < 3 {
            if original_entry & PD_ACCESS > 0 {
                address =
                    ((original_entry & 0xFFFF_FFFF_F000usize) + 0xFFFF_0000_0000_0000) as *mut u8;
                entry = None;
            } else {
                address = unsafe {
                    alloc::alloc::alloc(
                        Layout::from_size_align(4096, 4096).expect("Cannot allocate"),
                    )
                };
                entry = Some(
                    address.mask(0xFFFF_FFFF_FFFF) as usize
                        | PD_ACCESS
                        | AP_RW_EL0
                        | MAIR << 2
                        | PD_TABLE,
                );
            }
        } else {
            assert!(entry_vm_addr >= vm_addr);
            let offset = entry_vm_addr - vm_addr;

            address = unsafe { pm_addr.add(offset) };
            entry =
                Some(address.mask(0xFFFF_FFFF_FFFF) as usize | PD_ACCESS | AP_RW_EL0 | MAIR << 2 | 0b11);
        }

        // println!("entry: {:016x}", entry);
        // let test = ((entry >> 12) & 0xFFFF_FFFF) << 12;
        // println!("test: {:x}", test);
        // println!("addr: {:x}", address as usize);

        assert!(address.align_offset(4096) == 0, "Broken");
        // println!("");

        unsafe {
            if let Some(entry_data) = entry {
                write_volatile(entry_ptr, entry_data);
            }

            let vm = entry_vm_addr.max(vm_addr);
            let pm = pm_addr.add(vm - vm_addr);
            
            vm_setup_recursive(
                level + 1,
                address as *mut usize,
                entry_vm_addr,
                vm,
                pm,
                memory_size_per_entry.min(size),
                MAIR,
            );
        }
    }
}

pub fn vm_to_pm(vm_addr: usize) -> usize {
    let pgd_addr: usize;
    unsafe {
        if vm_addr < 0xFFFF_0000_0000_0000 {
            asm!(
                "mrs {PGD_addr}, ttbr0_el1",
                PGD_addr = out(reg) pgd_addr,
            );
        } else {
            asm!(
                "mrs {PGD_addr}, ttbr1_el1",
                PGD_addr = out(reg) pgd_addr,
            );
        }
    }
    vm_to_pm_addr_recursive(
        0,
        (pgd_addr + 0xFFFF_0000_0000_0000) as *mut u64,
        0,
        vm_addr,
    )
}

fn vm_to_pm_addr_recursive(
    level: u8,
    page_table: *mut u64,
    page_table_base: usize,
    vm_addr: usize,
) -> usize {
    let memory_size_per_entry: usize = match level {
        0 => 1024 * 1024 * 1024 * 512,
        1 => 1024 * 1024 * 1024,
        2 => 1024 * 1024 * 2,
        3 => 1024 * 4,
        _ => panic!("Cannot find level"),
    };

    let target_entry_idx = (vm_addr - page_table_base) / memory_size_per_entry;
    let offset = vm_addr - page_table_base - target_entry_idx * memory_size_per_entry;
    let entry = unsafe { read_volatile(page_table.add(target_entry_idx)) };
    let target_address = (entry & 0xFFFF_FFFF_F000u64) as usize;
    
    if level < 3 {
        vm_to_pm_addr_recursive(
            level + 1,
            (target_address + 0xFFFF_0000_0000_0000) as *mut u64,
            page_table_base + memory_size_per_entry * target_entry_idx,
            vm_addr,
        )
    } else {
        target_address + offset + 0xFFFF_0000_0000_0000
    }
}
