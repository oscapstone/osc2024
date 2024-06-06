use alloc::{rc::Rc, string::String};

use crate::{file_system::virtual_file_system, scheduler::current};

pub fn mount(
    src: *const i8,
    target: *const i8,
    file_system: *const i8,
    flags: u64,
    data: *const (),
) -> i32 {
    let current = unsafe { &mut *current() };
    let target = unsafe { core::ffi::CStr::from_ptr(target).to_str().unwrap() };
    let file_system = unsafe { core::ffi::CStr::from_ptr(file_system).to_str().unwrap() };
    let fs = match virtual_file_system().get_file_system(file_system) {
        Some(fs) => fs,
        None => return -1,
    };
    let mount = match fs.mount(Rc::downgrade(&fs), file_system) {
        Ok(mount) => mount,
        Err(_) => return -1,
    };
    let split = target.rsplit_once("/");
    // check if the mount path is already existed
    // if existed, remove the mount path
    if let Ok(dentry) = virtual_file_system().lookup(target) {
        let mount_directory_name = match split {
            Some((_, mount_directory_name)) => mount_directory_name,
            None => target,
        };
        dentry
            .parent()
            .unwrap()
            .upgrade()
            .unwrap()
            .remove_child(mount_directory_name);
        virtual_file_system().remove_directory_entry(&dentry.parent(), mount_directory_name);
    }
    // mount the file system
    let mount_root = mount.root.upgrade().unwrap();
    let mount_point = match split {
        Some((parent_path, mount_directory)) => {
            mount_root.set_name(String::from(mount_directory));
            if parent_path == "" {
                virtual_file_system()
                    .root()
                    .unwrap()
                    .root
                    .upgrade()
                    .unwrap()
            } else {
                match virtual_file_system().lookup(parent_path) {
                    Ok(dentry) => dentry,
                    Err(_) => return -1,
                }
            }
        }
        None => {
            mount_root.set_name(String::from(target));
            current
                .current_working_directory()
                .dentry
                .upgrade()
                .unwrap()
        }
    };
    mount_point.add_child(mount.root.clone());
    mount
        .root
        .upgrade()
        .unwrap()
        .set_parent(Some(Rc::downgrade(&mount_point)));
    virtual_file_system().update_directory_entry("/", mount_root);
    0
}
