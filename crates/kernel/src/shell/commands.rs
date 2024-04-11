use super::ShellCommand;
use crate::cpio::CpioArchive;
use crate::driver;
use small_std::{print, println};

pub struct HelloCommand;

impl ShellCommand for HelloCommand {
    fn name(&self) -> &str {
        "hello"
    }

    fn help(&self) -> &str {
        "print Hello World!"
    }

    fn execute(&self, _: &str) {
        println!("Hello World!");
    }
}

pub struct RebootCommand;

impl ShellCommand for RebootCommand {
    fn name(&self) -> &str {
        "reboot"
    }

    fn help(&self) -> &str {
        "reboot the device"
    }

    fn execute(&self, _: &str) {
        driver::watchdog().reset(100);
    }
}

pub struct InfoCommand;

impl ShellCommand for InfoCommand {
    fn name(&self) -> &str {
        "info"
    }

    fn help(&self) -> &str {
        "print hardware information"
    }

    fn execute(&self, _: &str) {
        let revision = driver::mailbox().get_board_revision();
        let memory = driver::mailbox().get_arm_memory();

        match revision {
            Ok(r) => println!("Board revision: {:#x}", r),
            Err(e) => println!("Failed to get board revision: {}", e),
        };
        match memory {
            Ok(m) => {
                println!("ARM Memory base address: {:#x}", m.base_address);
                println!("ARM Memory size: {:#x}", m.size);
            }
            Err(e) => println!("Failed to get memory info: {}", e),
        };
    }
}

pub struct LsCommand<'a> {
    cpio: &'a CpioArchive,
}

impl<'a> LsCommand<'a> {
    pub fn new(cpio: &'a CpioArchive) -> Self {
        Self { cpio }
    }
}

impl<'a> ShellCommand for LsCommand<'a> {
    fn name(&self) -> &str {
        "ls"
    }

    fn help(&self) -> &str {
        "list files in the initramfs"
    }

    fn execute(&self, _: &str) {
        for file in self.cpio.files() {
            println!("{}", file.filename);
        }
    }
}

pub struct CatCommand<'a> {
    pub cpio: &'a CpioArchive,
}

impl<'a> CatCommand<'a> {
    pub fn new(cpio: &'a CpioArchive) -> Self {
        Self { cpio }
    }
}

impl<'a> ShellCommand for CatCommand<'a> {
    fn name(&self) -> &str {
        "cat"
    }

    fn help(&self) -> &str {
        "print content of a file in the initramfs"
    }

    fn execute(&self, args: &str) {
        let mut has_args = false;
        for file in args.split_whitespace() {
            has_args = true;

            let mut found = false;
            for f in self.cpio.files() {
                if f.filename != file {
                    continue;
                }

                for c in f.content {
                    print!("{}", *c as char);
                }
                found = true;
                break;
            }

            if !found {
                println!("cat: {}: No such file or directory", file);
            }
        }

        if !has_args {
            println!("Usage: cat <file>...");
        }
    }
}
