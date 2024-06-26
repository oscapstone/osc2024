#![feature(asm_const)]
#![no_main]
#![no_std]
#![feature(default_alloc_error_handler)]
#![feature(format_args_nl)]
#![feature(int_log)]
#![feature(linked_list_cursors)]

mod bsp;
mod cpu;
mod fdt;
mod panic_wait;
mod print;
mod fs;
mod memory;
mod kernel_thread;
mod process;
mod interrupt;

use alloc::borrow::ToOwned;
use alloc::format;
use driver::uart::async_getline;
use fs::vfs::VFS;
use interrupt::timer;

extern crate alloc;

use core::alloc::GlobalAlloc;
use core::fmt::write;
use core::panic;
use alloc::string::String;
use alloc::string::ToString;
use alloc::vec::Vec;
use alloc::collections::BinaryHeap;
use alloc::boxed::Box;
use core::alloc::Layout;
use core::ptr::NonNull;
use fs::cpio::CpioHandler;
use fs::vfs::{FileSystemOperations};

use driver::mailbox;
use driver::uart;
use driver::addr_loader;
use kernel_thread::Thread;
use memory::{dynamic_memory_allocator, simple_alloc::SimpleAllocator};

#[global_allocator]
static mut ALLOCATOR: SimpleAllocator = SimpleAllocator::new();
static mut PAGE_ALLOC: dynamic_memory_allocator::DynamicMemoryAllocator = dynamic_memory_allocator::DynamicMemoryAllocator::new();

static mut GLOBAL_VFS: Option<VFS> = None;
static mut FSS: Vec<NonNull<dyn FileSystemOperations>> = Vec::new();

static mut CPIO_HANDLER: Option<CpioHandler> = None;

static mut PROCESS_SCHEDULER: Option<process::ProcessScheduler> = None;

fn get_initrd_addr(name: &str, data: *const u8, len: usize) -> Option<u32> {
    if name == "linux,initrd-start" {
        unsafe { Some((*(data as *const u32)).swap_bytes()) }
    } else {
        None
    }
}

fn foo(){
    let scheduler = kernel_thread::get_scheduler();
    for i in 0..10 {
        let cur_thread = scheduler.get_current_thread();
        println!("Thread id: {}, i: {}", cur_thread.get_thread_id(), i);
        kernel_thread::schedule();
    }
    exit_thread!();
}

#[no_mangle]
#[inline(never)]
fn exec(addr: extern "C" fn() -> !) {
    // set spsr_el1 to 0x3c0 and elr_el1 to the program’s start address.
    // set the user pro gram’s stack pointer to a proper position by setting sp_el0.
    // issue eret to return to the user code.
    unsafe {
        core::arch::asm!("
        msr spsr_el1, {k}
        msr elr_el1, {a}
        msr sp_el0, {s}
        eret
        ", k = in(reg) 0x3c0 as u64, a = in(reg) addr as u64, s = in(reg) 0x60000 as u64) ;
    };
    println!("");
}

#[no_mangle]
#[inline(never)]
fn boink() {
    unsafe{core::arch::asm!("nop")};
}


        
fn timer_callback(message: String, _: *mut u64) {
    let current_time = timer::get_current_time() / timer::get_timer_freq();
    println!("You have a timer after boot {}s, message: {}", current_time, message);
}

fn fs_open_shell(vfs: &mut VFS, path: &str) {
    let mut file = vfs.open(&path, true);
    match file {
        Ok(mut file) => {
            println!("File: {} opened.", path);
            loop {
                print!("file: {}>> ", path);
                let mut in_buf: [u8; 128] = [0; 128];
                let inp = alloc::string::String::from(uart::async_getline(&mut in_buf, true));
                let cmd = inp.trim().split(' ').collect::<Vec<&str>>();
                match cmd[0] {
                    "read" => {
                        let size = if cmd.len() < 2 {
                            32
                        } else {
                            cmd[1].parse().unwrap()
                        };
                        let read_buf = file.read(32);
                        match read_buf {
                            Ok(buf) => {
                                let s = String::from_utf8(buf).unwrap();
                                println!("{}", s);
                            }
                            Err(e) => {
                                println!("{}", e);
                            }
                        }
                    }
                    "write" => {
                        
                        let mut read_buf :[u8; 128] = [0; 128];
                        let read_line = async_getline(&mut read_buf, true);
                        let mut write_buf = Vec::new();
                        let bytes = read_line.as_bytes();
                        write_buf.extend_from_slice(bytes);
                        file.write(&write_buf);
                    }
                    "close" => {
                        // vfs.close(&file);
                        break;
                    }
                    "" => {}
                    _ => {
                        println!("Command not found");
                    }
                }
            }
        }
        Err(e) => {
            println!("{}", e);
        }
    }
}

fn fs_shell(idle_proc_code: &[u8]) {
    let mut vfs = unsafe {GLOBAL_VFS.as_mut().unwrap()};
    let mut working_dir = String::from("");
    let mut fss = unsafe {&mut FSS};
    loop {
        print!("{}>> ", &working_dir);
        let mut in_buf: [u8; 128] = [0; 128];
        let inp = alloc::string::String::from(uart::async_getline(&mut in_buf, true));
        let cmd = inp.trim().split(' ').collect::<Vec<&str>>();
        match cmd[0] {
            "help" => {
                println!("ls      : list files in directory\r\nmkdir   : create directory\r\nopen    : open file\r\ncat    : read and display file\r\nexit    : exit shell");
            }
            "ls" => {
                let path = if cmd.len() < 2 {
                    working_dir.to_owned()
                } else {
                    working_dir.to_owned() + "/" + cmd[1]
                };
                let ls = vfs.ls(&path);
                match ls {
                    Ok(files) => {
                        for file in files {
                            println!("{}", file);
                        }
                    }
                    Err(e) => {
                        println!("{}", e);
                    }
                }
            }
            "mkdir" => {
                if cmd.len() < 2 {
                    println!("Usage: mkdir <dir>");
                    continue;
                }
                let path = working_dir.to_owned() + "/" + cmd[1];
                let res = vfs.mkdir(&path);
                match res {
                    Ok(_) => {}
                    Err(e) => {
                        println!("{}", e);
                    }
                }
            }
            "cd" => {
                unimplemented!("cd");
            }
            "open" => {
                if cmd.len() < 2 {
                    println!("Usage: open <file>");
                    continue;
                }
                let path = working_dir.to_owned() + "/" + cmd[1];
                
                fs_open_shell(&mut vfs, &path);
            }
            "cat" => {
                if cmd.len() < 2 {
                    println!("Usage: cat <file>");
                    continue;
                }
                let path = working_dir.to_owned() + "/" + cmd[1];
                let mut file = vfs.open(&path, false);
                if let Err(e) = file {
                    println!("{}", e);
                    continue;
                }
                let mut  file = file.unwrap();
                while let Ok(buf) = file.read(2) {
                    let s = String::from_utf8(buf).unwrap();
                    print!("{}", s);
                }
                println!("");
                file.close();
            }
            "rmdir" => {
                unimplemented!("rmdir");
            }
            "mkfs" => {
                if cmd.len() < 2 {
                    println!("Usage: mkfs <name>");
                    continue;
                }
                let b = Box::new(fs::tmpfs::Tmpfs::new(cmd[1]));
                fss.push(NonNull::new(Box::into_raw(b)).unwrap());
            }
            "lsfs" => {
                for fs in fss.iter() {
                    let fs = unsafe {fs.as_ref()};
                    println!("{}", fs.get_name());
                }
            }
            "mount" => 
            {
                if cmd.len() < 3 {
                    println!("Usage: mount <fs> <path>");
                    continue;
                }
                let fs = cmd[1];
                let path = cmd[2];
                let fs = fss.iter().find(|f| {
                    let f = unsafe {f.as_ref()};
                    f.get_name() == fs
                });
                if let Some(fs) = fs {
                    vfs.mount(*fs, path);
                } else {
                    println!("Filesystem not found");
                }
            }
            "umount" => {
                if cmd.len() < 2 {
                    println!("Usage: umount <path>");
                    continue;
                }
                let path = cmd[1];
                vfs.umount(path);
            }
            "exec" => {
                if cmd.len() < 2 {
                    println!("Usage: exec <file>");
                    continue;
                }
                let file = vfs.open(cmd[1], false);
                match file {
                    Ok(mut file) => {
                        let mut data = Vec::new();
                        while let Ok(buf) = file.read(1024) {
                            data.extend_from_slice(&buf);
                        }
                        let process_scheduler = unsafe {PROCESS_SCHEDULER.as_mut().unwrap()};
                        let idle_pid = process_scheduler.create_process(&idle_proc_code);
                        println!("Idle process created with pid: {}", idle_pid);
                    
                        let pid = process_scheduler.create_process(&data);
                        println!("Process created with pid: {}", pid);
                        if process_scheduler.is_running() == false{
                            process_scheduler.start();
                        }
                    }
                    Err(e) => {
                        println!("{}", e);
                    }
                }
            }
            "exit" => {
                break;
            }
            _ => {
                println!("Command not found");
            }
        }
        
    }
}

fn init_fs(initrd_start: *mut u8) {
    // init vfs
    let mut fss = unsafe {&mut FSS};
    
    let b = Box::new(fs::tmpfs::Tmpfs::new("rootfs"));
    fss.push(NonNull::new(Box::into_raw(b)).unwrap());
    
    let b = Box::new(fs::tmpfs::Tmpfs::new("tmpfs"));
    fss.push(NonNull::new(Box::into_raw(b)).unwrap());
    
    let b = Box::new(fs::cpiofs::CpioFS::new("initramfs", initrd_start));
    fss.push(NonNull::new(Box::into_raw(b)).unwrap());
    
    let b = Box::new(fs::devfs::Devfs::new("devfs"));
    fss.push(NonNull::new(Box::into_raw(b)).unwrap());

    unsafe {GLOBAL_VFS = Some(VFS::new(fss[0]));}


}

#[no_mangle]
fn kernel_init() -> ! {
    let dtb_addr = addr_loader::load_dtb_addr();
    let dtb_parser = fdt::DtbParser::new(dtb_addr);
    // init process scheduler
    unsafe {
        PROCESS_SCHEDULER = Some(process::ProcessScheduler::new());
    }
    // init uart
    uart::init_uart(true);
    timer::init_timer();
    println_polling!("Kernel started");
    
    // find initrd value by dtb traverse
    
    let initrd_start: *mut u8;
    if let Some(addr) = dtb_parser.traverse(get_initrd_addr) {
        initrd_start = addr as *mut u8;
        println_polling!("Initrd start address: {:#x}", initrd_start as u64)
    } else {
        initrd_start = 0 as *mut u8;
    }
    unsafe {
        CPIO_HANDLER = Some(CpioHandler::new(initrd_start as *mut u8));
    }
    let cpio_handler = unsafe {CPIO_HANDLER.as_mut().unwrap()};
    
    // load symbol address usr_load_prog_base
    
    
    init_fs(initrd_start as *mut u8);


   
    let mut in_buf: [u8; 128] = [0; 128];
    
    let page_alloc = unsafe {&mut PAGE_ALLOC};
    page_alloc.init(0x00000000, 0x3C000000 , 4096);

    page_alloc.reserve_addr(0x0, 0x1000);
    page_alloc.reserve_addr(0x60000, 0x80000); // stack
    page_alloc.reserve_addr(0x80000, 0x100000); // code
    page_alloc.reserve_addr(initrd_start as usize, initrd_start as usize + 4096); //initrd
    page_alloc.reserve_addr(dtb_addr as usize, ((dtb_addr as usize + dtb_parser.get_dtb_size() as usize) + (4096 - 1)) & !(4096 - 1)); // dtb
    
    // let scheduler = kernel_thread::get_scheduler();
    let mut idle_proc_code = cpio_handler.get_files().find(|f| {
        f.get_name() == "prog"}
    ).unwrap();

    let idle_proc_data =  {
        let idle_proc_size = idle_proc_code.get_size();
        println!("Idle process size: {}", idle_proc_size);
        idle_proc_code.read(idle_proc_size)
    };

    loop {
        print!("meow>> ");
        let inp = alloc::string::String::from(uart::async_getline(&mut in_buf, true));
        let cmd = inp.trim().split(' ').collect::<Vec<&str>>();
        // split the input string
        match cmd[0] {
            "hello" => println!("Hello World!"),
            "help" => println!("help    : print this help menu\r\nhello   : print Hello World!\r\nreboot  : reboot the device"),
            "reboot" => {
                uart::reboot();
                break;
            }
            "ls" => {
                for file in cpio_handler.get_files() {
                    println!("{}, size: {}", file.get_name(), file.get_size());
                }
            }
            "cat" => {
                if cmd.len() < 2 {
                    println!("Usage: cat <file>");
                    continue;
                }
                let mut file = cpio_handler.get_files().find(|f| f.get_name() == cmd[1]);
                if let Some(mut f) = file {
                    println!("File size: {}", f.get_size());
                    loop {
                        let data = f.read(32);
                        if data.len() == 0 {
                            break;
                        }
                        for i in data {
                            unsafe {
                                uart::write_u8(*i);
                            }
                        }
                    }
                } else {
                    println!("File not found");
                }
            }
            "dtb" => {
                println!("Start address: {:#x}", dtb_addr as u64);
                println!("dtb load address: {:#x}", dtb_addr as u64);
                println!("dtb pos: {:#x}", unsafe {*(dtb_addr)});
                dtb_parser.parse_struct_block();
            }
            "mailbox" => {
                println!("Revision: {:#x}", mailbox::get_board_revisioin());
                let (base, size) = mailbox::get_arm_memory();
                println!("ARM memory base address: {:#x}", base);
                println!("ARM memory size: {:#x}", size);
            }
            "exec" => {
                if cmd.len() < 2 {
                    println!("Usage: exec <file>");
                    continue;
                }
                let file = cpio_handler.get_files().find(|f| {
                    f.get_name() == cmd[1]}
                );

                
                if let Some(mut f) = file {
                    let file_size = f.get_size();
                    println!("File size: {}", file_size);
                    let data = f.read(file_size);
                    let process_scheduler = unsafe {PROCESS_SCHEDULER.as_mut().unwrap()};
                    let idle_pid = process_scheduler.create_process(&idle_proc_data);
                    println!("Idle process created with pid: {}", idle_pid);
                    let pid = process_scheduler.create_process(&data);
                    
                    println!("Process created with pid: {}", pid);
                    if process_scheduler.is_running() == false{
                        process_scheduler.start();
                    }
                } else {
                    println!("File not found");
                }
            }
            "setTimeout" => {
                if cmd.len() < 3 {
                    println!("Usage: setTimeout <time>(s) <message>");
                    continue;
                }
                let time: u64 = cmd[1].parse().unwrap();
                let mesg = cmd[2].to_string();
                println!("Set timer after {}s, message :{}", time, mesg);                
                timer::add_timer(timer_callback, time * 1000, mesg);
            }
            "dalloc" => {
                if cmd.len() < 3 {
                    println!("Usage: dalloc <size> <allign>");
                    continue;
                }
                let size = match cmd[1].parse() {
                    Ok(s) => s,
                    Err(_) => {
                        println!("Invalid size");
                        continue;
                    }
                };
                let allign = match cmd[2].parse() {
                    Ok(s) => s,
                    Err(_) => {
                        println!("Invalid allign");
                        continue;
                    }
                };

                let alloc_addr = unsafe {page_alloc.alloc(Layout::from_size_align(size, allign).unwrap())} as usize;
                println!("Allocated address: {:#x}", alloc_addr);
            }
            "dfree" => {
                if cmd.len() < 2 {
                    println!("Usage: dfree <addr>");
                    continue;
                }
                let size: usize = cmd[1].parse().unwrap();
                unsafe {page_alloc.dealloc(size as *mut u8, Layout::from_size_align(1, 1).unwrap())};
            }
            "palloc" => {
                let size: usize = cmd[1].parse().unwrap();
                let pn = page_alloc.palloc(size) as usize;
                println!("Allocated page: {}", pn);
            }
            "pfree" => {
                let addr: usize = cmd[1].parse().unwrap();
                page_alloc.pfree(addr);
            }
            "show" => {
                if cmd.len() < 2 {
                    println!("Usage: show <d/p>");
                    continue;
                }   
                match cmd[1] {
                    "d" => {
                        page_alloc.dshow();
                    }
                    "p" => {
                        page_alloc.pshow();
                    }
                    _ => {
                        println!("Usage: show <d/p>");
                    }
                }
            }
            "rsv" => {
                if cmd.len() < 3 {
                    println!("Usage: rsv <start> <end>");
                    continue;
                }
                let start: usize = cmd[1].parse().unwrap();
                let end: usize = cmd[2].parse().unwrap();
                page_alloc.reserve_addr(start, end);
            }
            "fs" => {
                
                fs_shell(idle_proc_data);
            }
            "" => {}
            _ => {
                println!("Shell: Command not found");
            }
            
        }
    }


    panic!()
}
