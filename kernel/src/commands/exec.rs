use crate::INITRAMFS_ADDR;
use core::arch::asm;
use filesystem::cpio::CpioArchive;
use stdio::println;

const PROGRAM_ENTRY: *const u8 = 0x30010000 as *const u8;

pub fn exec(command: &[u8]) {
    let rootfs = CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
    let filename = &command[5..];
    if let Some(data) = rootfs.get_file(core::str::from_utf8(filename).unwrap()) {
        unsafe {
            core::ptr::copy(data.as_ptr(), PROGRAM_ENTRY as *mut u8, data.len());
            asm!(
                "mov {0}, 0x3c0",
                "msr spsr_el1, {0}",
                "msr elr_el1, {1}",
                "msr sp_el0, {2}",
                "eret",
                out(reg) _,
                in(reg) PROGRAM_ENTRY,
                in(reg) PROGRAM_ENTRY as u64 - 0x1000,
            );
        }
    } else {
        println!(
            "File not found: {}",
            core::str::from_utf8(filename).unwrap()
        );
    }
}
