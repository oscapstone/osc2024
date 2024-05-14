use crate::scheduler;
use crate::INITRAMFS_ADDR;
use alloc::alloc::alloc;
use alloc::alloc::Layout as layout;
use alloc::string::String;
use alloc::vec::Vec;
use filesystem::cpio::CpioArchive;
use stdio::println;

pub fn exec(args: Vec<String>) {
    println!("Executing exec command with args: {:?}", args);
    let rootfs = CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
    for filename in args.iter().skip(1) {
        let filename = filename.as_bytes();
        if let Some(data) = rootfs.get_file(core::str::from_utf8(filename).unwrap()) {
            let program_entry =
                unsafe { alloc(layout::from_size_align(data.len(), 0x100).unwrap()) };
            println!(
                "Executing program: {}",
                core::str::from_utf8(filename).unwrap()
            );
            println!("Program size: 0x{:x} bytes", data.len());
            println!("Program entry: {:p}", program_entry);
            let program_entry: extern "C" fn() = unsafe { core::mem::transmute(program_entry) };
            scheduler::get().create_thread(program_entry, data.len(), 0 as *mut u8);
            unsafe {
                core::ptr::copy(data.as_ptr(), program_entry as *mut u8, data.len());
            }
        } else {
            println!(
                "File not found: {}",
                core::str::from_utf8(filename).unwrap()
            );
        }
    }
    assert!(!scheduler::get().ready_queue.is_empty());
    scheduler::get().run_threads();
}
