use alloc::{rc::Rc, string::String};
use vfs::file::{Umode, UMODE};

use crate::{file_system::virtual_file_system, scheduler::current};

pub fn mkdir(path: *const i8, mode: u32) -> i32 {
    let current = unsafe { &mut *current() };
    let path = unsafe { core::ffi::CStr::from_ptr(path).to_str().unwrap() };
    match path.rsplit_once("/") {
        Some((parent_path, directory_name)) => {
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
            parent_node.inode().upgrade().unwrap().mkdir(
                Umode::new(UMODE::OWNER_READ::SET),
                String::from(directory_name),
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
                .mkdir(
                    Umode::new(UMODE::OWNER_READ::SET),
                    String::from(path),
                    Some(current.current_working_directory().dentry.clone()),
                );
        }
    }
    0
}
