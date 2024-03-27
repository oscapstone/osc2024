pub struct Dir {
    name: [u8; 32],
    files: Vec<File>,
    dirs: Vec<Dir>,
}
