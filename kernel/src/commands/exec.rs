use crate::scheduler;
use crate::INITRAMFS_ADDR;
use alloc::string::String;
use alloc::vec::Vec;
use filesystem::cpio::CpioArchive;
use stdio::println;

pub fn exec(args: Vec<String>) -> ! {
    println!("Executing exec command with args: {:?}", args);
    let rootfs = CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
    for filename in args.iter().skip(1) {
        let filename = filename.as_bytes();
        if let Some(data) = rootfs.get_file(core::str::from_utf8(filename).unwrap()) {
            let program = scheduler::alloc_prog(data);
            scheduler::get().create_thread(program);
        } else {
            println!(
                "File not found: {}",
                core::str::from_utf8(filename).unwrap()
            );
        }
    }

    if scheduler::get().ready_queue.is_empty() {
        panic!("No threads to run!");
    } else {
        scheduler::get().run_threads();
    }
}
