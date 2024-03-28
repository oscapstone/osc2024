use std::io::prelude::Write;
use std::thread::sleep;
use std::time::Duration;
use std::{env::args, fs::File, io::Read};

const HELP_MESSAGE: &str = "Usage: kernel-transfer <tty> <kernel image>";

fn main() -> Result<(), Box<dyn std::error::Error>> {
    if args().len() != 3 {
        return Err(HELP_MESSAGE.into());
    }
    let tty_path = args().nth(1).unwrap();
    let kernel_image_path = args().nth(2).unwrap();

    // read kernel image
    let mut kernel_image_file = File::open(kernel_image_path)?;
    let mut buf: Vec<u8> = Vec::new();
    let size = kernel_image_file.read_to_end(&mut buf)?;
    println!("Kernel size: {} bytes", size);

    // write to tty
    let mut tty = File::create(tty_path)?;
    println!("Sending protocal header...");
    tty.write_all(&size.to_le_bytes())?;
    let mut current = 0;
    // write 1024 bytes as a block to prevent data loss
    const BLOCK_SIZE: usize = 1024;
    println!("Sending kernel image...");
    while current + BLOCK_SIZE < buf.len() {
        tty.write_all(&buf[current..current + BLOCK_SIZE])?;
        current += BLOCK_SIZE;
        println!(
            "Progress: {}/{} ({:.1}%)",
            current,
            buf.len(),
            (current as f32 / buf.len() as f32) * 100.0
        );
        // prevent data loss
        sleep(Duration::from_millis(100));
    }
    tty.write_all(&buf[current..buf.len()])?;
    println!("Send complete.");
    Ok(())
}
