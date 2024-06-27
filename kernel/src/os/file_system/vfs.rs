use crate::alloc::string::ToString;
use crate::println;
use alloc::boxed::Box;
use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;

static mut FS: Vec<(String, Box<dyn FileSystem>)> = Vec::new(); // (name, fs)
static mut MOUNT_POINTS: Vec<(String, usize)> = Vec::new(); // (mount_point, fs_idx)
static mut FILE_HANDLES: Vec<FileHandle> = Vec::new();

pub struct FileHandle {
    pub path: String,
    pub offset: usize,
    pub fs: usize,
}

pub trait FileSystem {
    fn get_fs_name(&self) -> String;

    fn lookup(&self, path: &str) -> bool;
    fn create(&mut self, path: &str);
    fn open(&mut self, path: &str) -> bool;
    fn close(&mut self);

    fn read(&self, path: &str, offset: usize, buf: &mut [u8], len: usize) -> usize;
    fn write(&mut self, path: &str, offset: usize, buf: &[u8], len: usize) -> usize;
}

pub fn register_fs(fs: Box<dyn FileSystem>) {
    unsafe {
        // println!("Registered file system #{}: {}", FS.len(), &fs.get_fs_name());
        FS.push((fs.get_fs_name(), fs));
    }
}

pub fn mount(fs_name: &str, path: &str) {
    unsafe {
        let fs_idx = FS
            .iter()
            .position(|(name, _)| *name == fs_name)
            .expect(format!("File system not found: {}", fs_name).as_str());
        // println!("Mounting {} to {}", fs_name, path);
        MOUNT_POINTS.push((path.to_string(), fs_idx));
    }
}

fn get_fs(path: &str) -> Option<(usize, String)> {
    unsafe {
        let mut most_matched: Option<(usize, String)> = None; // (fs_idx, fs_path)

        for (mount_point, fs_idx) in MOUNT_POINTS.iter() {
            if path.starts_with(mount_point.as_str()) {
                let fs_path = path.replace(mount_point, "/");
                if most_matched.is_none() || fs_path.len() < most_matched.as_ref().unwrap().1.len() {
                    // println!("matching fs: {}", mount_point);
                    most_matched = Some((*fs_idx, fs_path));
                }
            }
        }

        most_matched
    }
}

pub fn path_process(path: &str) -> String {
    let mut path = path.to_string();

    let is_dir = path.ends_with("/");

    // Only support absolute path
    if !path.starts_with("/") {
        panic!("Only support absolute path");
    }

    // Remove duplicate slashes
    while path.contains("//") {
        path = path.replace("//", "/");
    }

    // Convert ".." and "." to absolute path
    let path_vec: Vec<&str> = path.split("/").collect();
    let mut new_path_vec: Vec<&str> = Vec::new();
    for i in 0..path_vec.len() {
        if path_vec[i] == ".." {
            new_path_vec.pop();
        } else if path_vec[i] != "." && path_vec[i] != "" {
            new_path_vec.push(path_vec[i]);
        }
    }

    let new_path = new_path_vec.join("/");
    if new_path == "" {
        "/".to_string()
    } else {
        if is_dir {
            format!("/{}/", new_path)
        } else {
            format!("/{}", new_path)
        }
    }
}

pub fn lookup(path: &str) -> bool {
    unsafe {
        let fs = get_fs(&path_process(&path));
        if let Some((fs_idx, fs_path)) = fs {
            if fs_path == "/" {
                return true;
            }
            FS[fs_idx].1.lookup(&fs_path)
        } else {
            false
        }
    }
}

pub fn create(path: &str) {
    unsafe {
        let fs = get_fs(&path_process(path));
        if let Some((fs_idx, fs_path)) = fs {
            FS[fs_idx].1.create(&fs_path);
        }
    }
}

pub fn open(path: &str) -> Option<usize> {
    unsafe {
        let fs = get_fs(&path_process(path));
        if let Some((fs_idx, fs_path)) = fs {
            if FS[fs_idx].1.open(&fs_path) {
                FILE_HANDLES.push(FileHandle {
                    path: fs_path,
                    offset: 0,
                    fs: fs_idx,
                });
                Some(FILE_HANDLES.len() - 1)
            } else {
                println!("VFS open: File not found");
                None
            }
        } else {
            None
        }
    }
}

pub fn close(fd: usize) {
    unsafe {
        let file = FILE_HANDLES.get(fd);
        if let Some(file) = file {
            FS[file.fs].1.close();
        }
    }
}

pub fn read(fd: usize, buf: &mut [u8], len: usize) -> usize {
    unsafe {
        let file = FILE_HANDLES.get_mut(fd);
        if let Some(file) = file {
            println!("fs_idx: {}", file.fs);
            let fs = &FS[file.fs].1;
            let count = fs.read(&file.path, file.offset, buf, len);
            file.offset += count;
            count
        } else {
            0
        }
    }
}

pub fn write(fd: usize, buf: &[u8], len: usize) -> usize {
    unsafe {
        let file = FILE_HANDLES.get_mut(fd);
        if let Some(file) = file {
            let fs = &mut FS[file.fs].1;
            let written_len = fs.write(&file.path, file.offset, buf, len);
            file.offset += written_len;
            written_len
        } else {
            0
        }
    }
}
