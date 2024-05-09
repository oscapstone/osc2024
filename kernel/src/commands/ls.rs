use crate::INITRAMFS_ADDR;
use filesystem::cpio::CpioArchive;

pub fn exec() {
    let rootfs = CpioArchive::load(unsafe { INITRAMFS_ADDR } as *const u8);
    rootfs.print_file_list();
}
