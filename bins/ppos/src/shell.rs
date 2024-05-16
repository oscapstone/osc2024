use core::arch::asm;
use core::time::Duration;

use aarch64_cpu::registers::*;
use alloc::string::String;
use alloc::vec::Vec;
use alloc::{boxed::Box, format};
use cpio::CPIOArchive;
use library::{console, print, println, sync::mutex::Mutex};

use crate::scheduler::current;
use crate::{
    driver::{self, mailbox},
    memory,
};

pub static mut SLAB_ALLOCATOR_DEBUG_ENABLE: bool = false;

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
            if let Some(c) = console::console().read_char() {
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
    }

    fn help(&self) {
        println!("help\t: print this help menu");
        println!("hello\t: print Hello World!");
        println!("reboot\t: reboot the device");
        println!("cancel-reboot\t: cancel reboot");
        println!("info\t: get hardware infomation");
        println!("ls\t: list files");
        println!("cat\t: show file content");
        println!("run-program\t: run a program (image)");
        println!("switch-2s-alert\t: enable/disable 2s alert");
        println!("set-timeout\t: print a message after period of time");
        println!("switch-slab-debug-mode\t: switch slab allocator debug mode");
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
                "run-program" => self.run_program(args),
                "test" => self.test(),
                "switch-2s-alert" => self.switch_2s_alert(),
                "set-timeout" => self.set_timeout(args),
                "switch-slab-debug-mode" => self.switch_slab_debug_mode(),
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

    fn run_program(&self, args: Box<[&str]>) {
        if args.len() != 1 {
            println!("Usage: run-program <file>");
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
                        if file.name != filename {
                            continue;
                        }
                        let mut code = Vec::from(file.content).into_boxed_slice();
                        let code_start = code.as_ptr();
                        Box::into_raw(code);
                        unsafe { &mut *current() }.run_user_program(code_start as *const fn() -> !);
                        return Ok(());
                    }
                    println!("run-program: {}: No such file or directory", filename);

                    return Ok(());
                }
                Ok(())
            })
            .unwrap();
    }

    fn switch_2s_alert(&mut self) {
        let mut enable_2s_alert = ALERT_HANDLER.enable_2s_alert.lock().unwrap();
        match *enable_2s_alert {
            true => {
                *enable_2s_alert = false;
            }
            false => {
                driver::timer().set_timeout(
                    Duration::from_secs(2),
                    Box::new(|| ALERT_HANDLER.handle_timeout()),
                );
                *enable_2s_alert = true;
            }
        }
    }

    fn set_timeout(&self, args: Box<[&str]>) {
        if args.len() != 2 {
            println!("Usage: set-timeout <message> <secord>");
            return;
        }
        let secords = args[1].parse();
        if secords.is_err() {
            println!("Usage: set-timeout <message> <secord>");
            return;
        }
        let secords = secords.unwrap();
        let message = String::from(args[0]);
        driver::timer().set_timeout(
            Duration::from_secs(secords),
            Box::new(move || {
                println!("{}", message);
                Ok(())
            }),
        );
    }

    fn test(&self) {
        driver::timer().set_timeout(
            Duration::from_secs(1),
            Box::new(|| {
                unsafe { TEST = true };
                let mut i = 0;
                while i < 100000000 {
                    i += 1;
                }
                println!("done");
                unsafe { TEST = false };
                Ok(())
            }),
        )
    }

    fn switch_slab_debug_mode(&self) {
        unsafe { SLAB_ALLOCATOR_DEBUG_ENABLE = !SLAB_ALLOCATOR_DEBUG_ENABLE };
    }
}

struct AlertHandler {
    enable_2s_alert: Mutex<bool>,
}

impl AlertHandler {
    fn handle_timeout(&self) -> Result<(), &'static str> {
        if *self.enable_2s_alert.lock().unwrap() {
            println!(
                "{} seconds after booting",
                CNTPCT_EL0.get() / CNTFRQ_EL0.get()
            );
            driver::timer().set_timeout(
                Duration::from_secs(2),
                Box::new(|| ALERT_HANDLER.handle_timeout()),
            );
        }
        Ok(())
    }
}

static ALERT_HANDLER: AlertHandler = AlertHandler {
    enable_2s_alert: Mutex::new(false),
};

pub static mut TEST: bool = false;
