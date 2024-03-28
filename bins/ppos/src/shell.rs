use alloc::boxed::Box;
use cpio::CPIOArchive;
use library::{console, format, print, println, string::String};

use crate::{
    driver::{self, mailbox},
    memory,
};

pub struct Shell {
    input: String,
}

impl Default for Shell {
    fn default() -> Self {
        Shell::new()
    }
}

impl Shell {
    pub fn new() -> Self {
        Self {
            input: String::from(""),
        }
    }

    fn shell_hint(&self) {
        print!("# ");
    }

    pub fn run(&mut self) -> ! {
        self.shell_hint();
        loop {
            let c = console::console().read_char();
            match c {
                '\r' | '\n' => {
                    self.execute_command();
                    self.shell_hint();
                }
                '\x08' | '\x7f' => self.backspace(),
                ' '..='~' => self.press_key(c),
                _ => (),
            }
        }
    }

    fn help(&self) {
        println!("help\t: print this help menu");
        println!("hello\t: print Hello World!");
        println!("reboot\t: reboot the device");
        println!("cancel-reboot\t: cancel reboot");
        println!("info\t: get hardware infomation");
        println!("ls\t: list files");
        println!("cat\t: show file content");
    }

    fn reboot(&self) {
        driver::watchdog().reset(0x20);
    }

    fn cancel_reboot(&self) {
        driver::watchdog().cancel_reset();
    }

    fn hello(&self) {
        println!("Hello World!");
    }

    fn info(&self) {
        let mem_info = mailbox().get_arm_memory();
        println!("board revision: {:#08x}", mailbox().get_board_revision());
        println!("ARM memory base address: {:#08x}", mem_info.base_address);
        println!("ARM memory size: {} bytes", mem_info.size);
    }

    fn execute_command(&mut self) {
        println!();
        let input = self.input.trim();
        let mut split_result = input.split(" ");
        if let Some(cmd) = split_result.next() {
            let args = split_result.collect::<Box<[&str]>>();
            match cmd {
                "help" => self.help(),
                "hello" => self.hello(),
                "reboot" => self.reboot(),
                "cancel-reboot" => self.cancel_reboot(),
                "info" => self.info(),
                "ls" => self.ls(),
                "cat" => self.cat(args),
                "" => (),
                cmd => println!("{}: command not found", cmd),
            }
        }
        self.input.clear();
    }

    fn press_key(&mut self, key: char) {
        self.input.push(key);
        print!("{}", key);
    }

    fn backspace(&mut self) {
        if self.input.is_empty() {
            return;
        }
        self.input.pop();
        // move the cursor to the previous character and overwrite it with a space
        // then move the cursor back again
        print!("\x08 \x08");
    }

    fn ls(&self) {
        let mut devicetree =
            unsafe { devicetree::FlattenedDevicetree::from_memory(memory::DEVICETREE_START_ADDR) };
        devicetree
            .traverse(&|device_name, property_name, property_value| {
                if property_name == "linux,initrd-start" {
                    let mut cpio_archive = unsafe {
                        CPIOArchive::from_memory(u32::from_be_bytes(
                            property_value.try_into().unwrap(),
                        ) as usize)
                    };
                    while let Some(file) = cpio_archive.read_next() {
                        println!("{}", file.name);
                    }
                }
                Ok(())
            })
            .unwrap();
    }

    fn cat(&self, args: Box<[&str]>) {
        if args.len() != 1 {
            println!("Usage: cat <file>");
            return;
        }

        let t = format!("{}\0", args[0]);
        let filename = t.as_str();
        let mut devicetree =
            unsafe { devicetree::FlattenedDevicetree::from_memory(memory::DEVICETREE_START_ADDR) };
        devicetree
            .traverse(&move |device_name, property_name, property_value| {
                if property_name == "linux,initrd-start" {
                    let mut cpio_archive = unsafe {
                        CPIOArchive::from_memory(u32::from_be_bytes(
                            property_value.try_into().unwrap(),
                        ) as usize)
                    };
                    while let Some(file) = cpio_archive.read_next() {
                        if file.name == filename {
                            for byte in file.content {
                                print!("{}", *byte as char);
                            }
                            return Ok(());
                        }
                    }
                    println!("cat: {}: No such file or directory", filename);
                    return Ok(());
                }
                Ok(())
            })
            .unwrap();
    }
}
