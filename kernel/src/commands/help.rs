use stdio::println;
pub fn exec() {
    let width = 12;
    println!("{:width$}: {}", "help", "print this help menu");
    println!("{:width$}: {}", "hello", "print Hello, world!");
    println!("{:width$}: {}", "reboot", "reboot the Raspberry Pi");
    println!("{:width$}: {}", "ls", "list files in the initramfs");
    println!(
        "{:width$}: {}",
        "cat", "print the content of a file in the initramfs"
    );
    println!(
        "{:width$}: {}",
        "exec", "execute a program in the initramfs"
    );
    println!(
        "{:width$}: {}",
        "setTimeOut", "print a message after some time"
    );
    println!(
        "{:width$}: {}",
        "buddy", "interact with the dangerous buddy allocator"
    );
}
