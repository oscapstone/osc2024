use stdio::println;

pub fn exec(command: &[u8]) {
    for c in &command[5..] {
        driver::uart::send_async(*c);
    }
    println!("Echoed: {}", core::str::from_utf8(&command[5..]).unwrap());
    loop {
        if let Some(c) = driver::uart::recv_async() {
            println!("Received: {} (0x{:x})", c as char, c);
            if c == b'\n' {
                break;
            }
        }
    }
}
