use crate::{file_system::virtual_file_system, scheduler::current};

pub enum Whence {
    Set = 0,
    Current = 1,
    End = 2,
}

impl From<i32> for Whence {
    fn from(value: i32) -> Self {
        match value {
            0 => Whence::Set,
            1 => Whence::Current,
            2 => Whence::End,
            _ => panic!("Invalid whence value"),
        }
    }
}

pub fn lseek64(fd: i32, offset: i64, whence: Whence) -> i64 {
    let current = unsafe { &mut *current() };
    let file_descriptor = match current.get_file(fd as usize) {
        Ok(file) => file,
        Err(_) => return -1,
    };
    let file_handle = match virtual_file_system().get_file(file_descriptor.file_handle_index()) {
        Ok(file) => file,
        Err(_) => return -1,
    };
    match whence {
        Whence::Set => {
            if offset < 0 || offset as usize > file_handle.inode().size() {
                return -1;
            }
            file_handle.set_position(offset as usize);
            offset
        }
        Whence::Current => {
            let target = file_handle.position() as i64 + offset;
            if target < 0 || target > file_handle.inode().size() as i64 {
                return -1;
            }
            file_handle.set_position(target as usize);
            target
        }
        Whence::End => {
            if offset > 0 {
                return -1;
            }
            let target = file_handle.inode().size() as i64 + offset;
            if target < 0 {
                return -1;
            }
            file_handle.set_position(target as usize);
            target
        }
    }
}
