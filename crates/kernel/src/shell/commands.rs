use super::ShellCommand;
use crate::driver;
use small_std::println;

pub struct HelloCommand;

impl ShellCommand for HelloCommand {
    fn name(&self) -> &str {
        "hello"
    }

    fn help(&self) -> &str {
        "print Hello World!"
    }

    fn execute(&self) {
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

    fn execute(&self) {
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

    fn execute(&self) {
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
