use aarch64_cpu::registers::Readable;
use alloc::{rc::Rc, string::String};
use tock_registers::{register_bitfields, registers::InMemoryRegister};
use vfs::file::{Umode, UMODE};

use crate::{file_system::virtual_file_system, scheduler::current};

register_bitfields! [
    u32,
    OPEN_FLAG [
        CREATE OFFSET(6) NUMBITS(1) [],
    ]
];

pub fn open(path: *const i8, flags: u32) -> i32 {
    let current = unsafe { &mut *current() };
    let path = unsafe { core::ffi::CStr::from_ptr(path).to_str().unwrap() };
    let flags = InMemoryRegister::<u32, OPEN_FLAG::Register>::new(flags);
    if flags.is_set(OPEN_FLAG::CREATE) && virtual_file_system().lookup(path).is_err() {
        match path.rsplit_once("/") {
            Some((parent_path, file_name)) => {
                let parent_node = if parent_path == "" {
                    virtual_file_system()
                        .root()
                        .unwrap()
                        .root
                        .upgrade()
                        .unwrap()
                } else {
                    virtual_file_system().lookup(parent_path).unwrap()
                };
                parent_node.inode().upgrade().unwrap().create(
                    Umode::new(UMODE::OWNER_READ::SET),
                    String::from(file_name),
                    Some(Rc::downgrade(&parent_node)),
                );
            }
            None => {
                current
                    .current_working_directory()
                    .dentry
                    .upgrade()
                    .unwrap()
                    .inode()
                    .upgrade()
                    .unwrap()
                    .create(
                        Umode::new(UMODE::OWNER_READ::SET),
                        String::from(path),
                        Some(current.current_working_directory().dentry.clone()),
                    );
            }
        }
    }
    match current.open_file(path) {
        Ok(fd) => fd as i32,
        Err(_) => -1,
    }
}
