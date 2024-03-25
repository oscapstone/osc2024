use crate::println;

const INITRAMFS_POS: u64 = 0x800_0000;

// FIXME:: use bincode to rewire following dirty code

#[repr(packed)]
pub struct CpioNewcHeader {
    c_magic: [u8; 6],
    c_ino: [u8; 8],
    c_mode: [u8; 8],
    c_uid: [u8; 8],
    c_gid: [u8; 8],
    c_nlink: [u8; 8],
    c_mtime: [u8; 8],
    c_filesize: [u8; 8],
    c_dev_maj: [u8; 8],
    c_dev_min: [u8; 8],
    c_rdev_maj: [u8; 8],
    c_rdev_min: [u8; 8],
    c_namesize: [u8; 8],
    c_check: [u8; 8],
}

pub fn list_initramfs_files() {
    let mut initramfs_ptr = INITRAMFS_POS as *const u8;
    let cwd = "rootfs";
    let mut header: &CpioNewcHeader;
    loop {
        header = unsafe { &*(initramfs_ptr as *const CpioNewcHeader) };
        if &header.c_magic != b"070701" {
            break;
        }
        // let inode = u32::from_str_radix(core::str::from_utf8(&header.c_ino).unwrap(), 16).unwrap();
        // let file_mode = u32::from_str_radix(core::str::from_utf8(&header.c_mode).unwrap(), 16).unwrap();
        // let file_owner = u32::from_str_radix(core::str::from_utf8(&header.c_uid).unwrap(), 16).unwrap();
        // let file_group = u32::from_str_radix(core::str::from_utf8(&header.c_gid).unwrap(), 16).unwrap();
        let file_size = u32::from_str_radix(core::str::from_utf8(&header.c_filesize).unwrap(), 16).unwrap();
        let filename_size = u32::from_str_radix(core::str::from_utf8(&header.c_namesize).unwrap(), 16).unwrap();
        let filename = unsafe { core::slice::from_raw_parts(initramfs_ptr.add(110), filename_size as usize) };
        let filename_padding = (4 - ((filename_size + 110) % 4)) % 4;
        // let file_content = unsafe { core::slice::from_raw_parts(initramfs_ptr.add(110 + filename_size as usize + filename_padding as usize), file_size as usize) };
        let file_content_padding = (4 - (file_size % 4)) % 4;

        let mut filename = core::str::from_utf8(filename).unwrap();
        if filename == "TRAILER!!!\0" {
            break;
        }
        if filename.starts_with(cwd) {
            filename = filename.split(cwd).last().unwrap();
            filename = filename.split("/").last().unwrap();
        }

        println!("{}", filename);
        // print all
        // println!("inode: {:x}, mode: {:x}, owner: {:x}, group: {:x}, size: {:x}, filename: {}, file_content_padding: {}", inode, file_mode, file_owner, file_group, file_size, core::str::from_utf8(filename).unwrap(), file_content_padding);
        initramfs_ptr = unsafe { initramfs_ptr.add(110 + filename_size as usize + filename_padding as usize + file_size as usize + file_content_padding as usize) };
    }
}


pub fn get_initramfs_files(cur_filename: &str) {
    let mut initramfs_ptr = INITRAMFS_POS as *const u8;
    let cwd = "rootfs";
    let mut header: &CpioNewcHeader;
    loop {
        header = unsafe { &*(initramfs_ptr as *const CpioNewcHeader) };
        if &header.c_magic != b"070701" {
            break;
        }
        let file_size = u32::from_str_radix(core::str::from_utf8(&header.c_filesize).unwrap(), 16).unwrap();
        let filename_size = u32::from_str_radix(core::str::from_utf8(&header.c_namesize).unwrap(), 16).unwrap();
        let filename = unsafe { core::slice::from_raw_parts(initramfs_ptr.add(110), filename_size as usize) };
        let filename_padding = (4 - ((filename_size + 110) % 4)) % 4;
        let file_content = unsafe { core::slice::from_raw_parts(initramfs_ptr.add(110 + filename_size as usize + filename_padding as usize), file_size as usize) };
        let file_content_padding = (4 - (file_size % 4)) % 4;

        let mut filename = core::str::from_utf8(filename).unwrap();
        filename = filename.split("\0").next().unwrap();
        if filename == "TRAILER!!!\0" {
            println!("not found!!\0");
            break;
        }


        // abs path
        if cur_filename.starts_with("/") {
            let cur_filename = &cur_filename[1..];
            if cur_filename == filename {
                println!("{}", core::str::from_utf8(file_content).unwrap());
                break;
            }
        } else if filename.starts_with(cwd) {
            filename = filename.split(cwd).last().unwrap();
            filename = filename.split("/").last().unwrap();
            if cur_filename == filename {
                println!("{}", core::str::from_utf8(file_content).unwrap());
                break;
            }
        }

        // println!("inode: {:x}, mode: {:x}, owner: {:x}, group: {:x}, size: {:x}, filename: {}, file_content_padding: {}", inode, file_mode, file_owner, file_group, file_size, core::str::from_utf8(filename).unwrap(), file_content_padding);
        initramfs_ptr = unsafe { initramfs_ptr.add(110 + filename_size as usize + filename_padding as usize + file_size as usize + file_content_padding as usize) };
    }
}
