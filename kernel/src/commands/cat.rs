use crate::INITRAMFS_ADDR;
use filesystem::cpio::CpioArchive;
use stdio::print;
use stdio::println;

pub fn exec(command: &[u8]) {
    let rootfs = CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
    let filename = &command[4..];
    if let Some(data) = rootfs.get_file(core::str::from_utf8(filename).unwrap()) {
        print!("{}", core::str::from_utf8(data).unwrap());
    } else {
        println!(
            "File not found: {}",
            core::str::from_utf8(filename).unwrap()
        );
    }
}
