use alloc::rc::Rc;

use crate::{
    file_system::{path::Path, virtual_file_system, VFSMount},
    scheduler::current,
};

pub fn chdir(path: *const i8) -> i32 {
    let current = unsafe { &mut *current() };
    let path = unsafe { core::ffi::CStr::from_ptr(path).to_str().unwrap() };
    let dentry = match virtual_file_system().lookup(path) {
        Ok(dentry) => dentry,
        Err(_) => return -1,
    };
    current.set_current_working_directory(Path::new(
        VFSMount::new(Rc::downgrade(&dentry), dentry.super_block()),
        Rc::downgrade(&dentry),
    ));
    0
}
