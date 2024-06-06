use super::path::Path;

#[derive(Debug, Clone)]
pub struct FileSystemInfo {
    pub root: Path,
    pub current_working_directory: Path,
}

impl FileSystemInfo {
    pub const fn new(root: Path, current_working_directory: Path) -> Self {
        Self {
            root,
            current_working_directory,
        }
    }
}
