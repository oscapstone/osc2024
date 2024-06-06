use core::time::Duration;

use aarch64_cpu::registers::*;
use alloc::boxed::Box;
use alloc::rc::Rc;
use alloc::string::String;
use alloc::vec;
use library::{console, print, println, sync::mutex::Mutex};
use vfs::file::{Umode, UMODE};

use crate::driver::{self, mailbox};
use crate::file_system::path::Path;
use crate::file_system::{virtual_file_system, VFSMount};
use crate::scheduler::current;

pub static mut SLAB_ALLOCATOR_DEBUG_ENABLE: bool = false;
pub static mut BUDDY_ALLOCATOR_DEBUG_ENABLE: bool = false;

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
        println!("cd\t: change directory");
        println!("cat\t: show file content");
        println!("mkdir\t: create a directory");
        println!("touch\t: create a file");
        println!("run-program\t: run a program (image)");
        println!("switch-2s-alert\t: enable/disable 2s alert");
        println!("set-timeout\t: print a message after period of time");
        println!("switch-slab-debug-mode\t: switch slab allocator debug mode");
        println!("switch-buddy-debug-mode\t: switch buddy allocator debug mode");
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
                "cd" => self.cd(args),
                "cat" => self.cat(args),
                "mkdir" => self.mkdir(args),
                "touch" => self.touch(args),
                "run-program" => self.run_program(args),
                "test" => self.test(),
                "switch-2s-alert" => self.switch_2s_alert(),
                "set-timeout" => self.set_timeout(args),
                "switch-slab-debug-mode" => self.switch_slab_debug_mode(),
                "switch-buddy-debug-mode" => self.switch_buddy_debug_mode(),
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
        let current = unsafe { &*current() };
        for entry in current
            .current_working_directory()
            .dentry
            .upgrade()
            .unwrap()
            .children()
        {
            println!("{}", entry.upgrade().unwrap().name());
        }
    }

    fn cd(&self, args: Box<[&str]>) {
        if args.len() != 1 {
            println!("Usage: cd <directory>");
            return;
        }
        let directory_name = args[0];
        let current = unsafe { &mut *current() };

        match virtual_file_system().lookup(directory_name) {
            Ok(dir) => current.set_current_working_directory(Path::new(
                VFSMount::new(
                    dir.super_block().upgrade().unwrap().root().unwrap(),
                    dir.super_block().clone(),
                ),
                Rc::downgrade(&dir),
            )),
            Err(e) => {
                println!("cd: {}", e);
            }
        }
    }

    fn cat(&self, args: Box<[&str]>) {
        if args.len() != 1 {
            println!("Usage: cat <file>");
            return;
        }

        let filename = args[0];
        let current = unsafe { &mut *current() };

        match current.open_file(filename) {
            Ok(fd) => {
                let file_descriptor = current.get_file(fd).unwrap();
                let file = virtual_file_system()
                    .get_file(file_descriptor.file_handle_index())
                    .unwrap();
                let file_size = file.inode().size();
                let mut buf = vec![0_u8; file_size].into_boxed_slice();
                let len = file.read(&mut buf, file_size).unwrap();
                current.close_file(fd).unwrap();
                if len != file_size {
                    println!("cat: {}: File is corrupted", filename);
                    return;
                }
                for i in 0..len {
                    print!("{}", buf[i] as char);
                }
            }
            Err(e) => {
                println!("cat: {}", e);
            }
        }
    }

    fn run_program(&self, args: Box<[&str]>) {
        if args.len() != 1 {
            println!("Usage: run-program <file>");
            return;
        }

        let filename = args[0];
        let current = unsafe { &mut *current() };
        match current.open_file(filename) {
            Ok(fd) => {
                let file_descriptor = current.get_file(fd).unwrap();
                let file = virtual_file_system()
                    .get_file(file_descriptor.file_handle_index())
                    .unwrap();
                let file_size = file.inode().size();
                let mut buf = vec![0_u8; file_size].into_boxed_slice();
                let len = file.read(&mut buf, file_size).unwrap();
                if len != file_size {
                    println!("run-program: {}: File is corrupted", filename);
                    return;
                }
                current.run_user_program(&buf);
            }
            Err(e) => {
                println!("run-program: {}", e);
            }
        }
    }

    fn mkdir(&self, args: Box<[&str]>) {
        if args.len() != 1 {
            println!("Usage: mkdir <directory>");
            return;
        }
        let current = unsafe { &mut *current() };
        match args[0].rsplit_once("/") {
            Some((parent_path, directory_name)) => {
                let parent_node = if parent_path == "" {
                    virtual_file_system()
                        .root()
                        .unwrap()
                        .root
                        .upgrade()
                        .unwrap()
                } else {
                    virtual_file_system().lookup(parent_path).unwrap()
                };
                parent_node.inode().upgrade().unwrap().mkdir(
                    Umode::new(UMODE::OWNER_READ::SET),
                    String::from(directory_name),
                    Some(Rc::downgrade(&parent_node)),
                );
            }
            None => {
                current
                    .current_working_directory()
                    .dentry
                    .upgrade()
                    .unwrap()
                    .inode()
                    .upgrade()
                    .unwrap()
                    .mkdir(
                        Umode::new(UMODE::OWNER_READ::SET),
                        String::from(args[0]),
                        Some(current.current_working_directory().dentry.clone()),
                    );
            }
        }
    }

    fn touch(&self, args: Box<[&str]>) {
        if args.len() != 1 {
            println!("Usage: touch <file>");
            return;
        }
        let current = unsafe { &mut *current() };
        match args[0].rsplit_once("/") {
            Some((parent_path, filename)) => {
                let parent_node = virtual_file_system().lookup(parent_path).unwrap();
                parent_node.inode().upgrade().unwrap().create(
                    Umode::new(UMODE::OWNER_READ::SET),
                    String::from(filename),
                    Some(Rc::downgrade(&parent_node)),
                );
            }
            None => {
                current
                    .current_working_directory()
                    .dentry
                    .upgrade()
                    .unwrap()
                    .inode()
                    .upgrade()
                    .unwrap()
                    .create(
                        Umode::new(UMODE::OWNER_READ::SET),
                        String::from(args[0]),
                        Some(current.current_working_directory().dentry.clone()),
                    );
            }
        }
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

    fn switch_buddy_debug_mode(&self) {
        unsafe { BUDDY_ALLOCATOR_DEBUG_ENABLE = !BUDDY_ALLOCATOR_DEBUG_ENABLE };
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
