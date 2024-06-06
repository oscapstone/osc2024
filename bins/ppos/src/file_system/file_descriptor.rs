#[derive(Debug, Clone)]
pub struct FileDescriptor {
    file_handle_index: usize,
}

impl FileDescriptor {
    pub const fn new(file_handle_index: usize) -> Self {
        Self { file_handle_index }
    }

    pub fn file_handle_index(&self) -> usize {
        self.file_handle_index
    }
}
