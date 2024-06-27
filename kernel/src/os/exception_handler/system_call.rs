use super::super::{file_system::vfs, thread};
use super::trap_frame;
use crate::os::file_system::vfs::path_process;
use crate::{
    cpu::{mmu::vm_to_pm, uart},
    os::stdio::println_now,
};
use alloc::vec;
use alloc::{format, slice};
use alloc::{string::String, vec::Vec};
use core::ptr::{read_volatile, write_volatile};

#[no_mangle]
#[inline(never)]
pub unsafe fn get_pid(trap_frame_ptr: *mut u64, current_pc: u64) {
    let pid = thread::get_running_thread_id().expect("Failed to get PID");
    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, pid as u64);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn read_from_uart(trap_frame_ptr: *mut u64) {
    let buf = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;
    let size = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as usize;

    let mut idx = 0;
    while idx < size {
        match uart::recv_async() {
            Some(byte) => {
                write_volatile(buf.add(idx), byte);
                idx += 1;
            }
            None => {
                if idx == 0 {
                    let pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC);
                    trap_frame::set(trap_frame_ptr, trap_frame::Register::PC, pc - 4);
                    thread::context_switching();
                }
                break;
            }
        }
    }

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, idx as u64);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn write_to_uart(trap_frame_ptr: *mut u64) {
    let buf = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *const u8;
    let size = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as usize;

    for i in 0..size {
        uart::send_async(read_volatile(buf.add(i)));
    }

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, size as u64);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn exec(trap_frame_ptr: *mut u64) {
    let mut program_name_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;
    let program_args_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *mut u8;

    let mut program_name = String::new();
    loop {
        let byte = read_volatile(program_name_ptr);
        if byte == 0 {
            break;
        }
        program_name.push(byte as char);
        program_name_ptr = program_name_ptr.add(1);
    }

    thread::exec(program_name, Vec::new());
    thread::restore_context(trap_frame_ptr);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn fork(trap_frame_ptr: *mut u64) {
    thread::save_context(trap_frame_ptr);
    match thread::fork() {
        Some(pid) => trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, pid as u64),
        None => panic!("Fork failed"),
    }
}

#[no_mangle]
#[inline(never)]
pub unsafe fn exit(trap_frame_ptr: *mut u64) {
    let pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC) as usize;
    let pid = thread::get_running_thread_id().expect("Failed to get PID");
    thread::kill(pid);
    thread::switch_to_thread(None, trap_frame_ptr);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn mailbox_call(trap_frame_ptr: *mut u64) {
    // mail box
    let channel = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as u8;
    let mbox_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *mut u32;

    let mbox_pm = vm_to_pm(mbox_ptr as usize);
    println_now(format!("PM: {:X}", mbox_pm).as_str());

    crate::cpu::mailbox::mailbox_call(channel, mbox_pm as *mut u32);

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, 1);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn kill(trap_frame_ptr: *mut u64) {
    let pid = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as usize;

    let my_pid = thread::get_running_thread_id().expect("Failed to get PID");

    if my_pid != pid {
        thread::kill(pid);
    } else {
        println!("Cannot kill yourself");
    }
}

#[no_mangle]
#[inline(never)]
pub unsafe fn open(trap_frame_ptr: *mut u64) {
    let path_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;
    let flags = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as u32;

    let path = char_ptr_to_string(path_ptr);
    let path = running_thread_full_path(path);

    // Create file
    if flags == 0x40 {
        vfs::create(path.as_str());
    }

    let fd = vfs::open(path.as_str()).expect("Failed to open file");
    let fd = running_thread_new_fd(fd);

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, fd as u64);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn close(trap_frame_ptr: *mut u64) {
    let fd = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as usize;

    let fd = running_thread_fd_lookup(fd);
    vfs::close(fd);

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, 0);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn write(trap_frame_ptr: *mut u64) {
    let fd = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as usize;
    let buf = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *const u8;
    let len = trap_frame::get(trap_frame_ptr, trap_frame::Register::X2) as usize;

    let fd = running_thread_fd_lookup(fd);

    let written_len = vfs::write(fd, slice::from_raw_parts(buf, len), len);
    assert!(written_len <= len);

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, written_len as u64);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn read(trap_frame_ptr: *mut u64) {
    let fd = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as usize;
    let buf = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *mut u8;
    let len = trap_frame::get(trap_frame_ptr, trap_frame::Register::X2) as usize;

    let fd = running_thread_fd_lookup(fd);
    let buf_arr = slice::from_raw_parts_mut(buf, len);

    let read_len = vfs::read(fd, buf_arr, len);
    assert!(read_len <= len);

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, read_len as u64);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn mkdir(trap_frame_ptr: *mut u64) {
    let path_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;
    let _mode = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as u32; // unused

    let path = char_ptr_to_string(path_ptr);

    vfs::create(path.as_str());

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, 0);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn mount(trap_frame_ptr: *mut u64) {
    // unimplemented!("MOUNT system call is not implemented");
    let _src = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;
    let target_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as *mut u8;
    let fs_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X2) as *mut u8;
    let _flags = trap_frame::get(trap_frame_ptr, trap_frame::Register::X3) as u32;
    let _data = trap_frame::get(trap_frame_ptr, trap_frame::Register::X4) as *mut u8;

    let target = char_ptr_to_string(target_ptr);
    let fs = char_ptr_to_string(fs_ptr);

    vfs::mount(fs.as_str(), target.as_str());

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, 0);
}

#[no_mangle]
#[inline(never)]
pub unsafe fn chdir(trap_frame_ptr: *mut u64) {
    let path_ptr = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as *mut u8;

    let path = char_ptr_to_string(path_ptr);
    let path = path_process(path.as_str());

    let pid = thread::get_running_thread_id().expect("Failed to get running thread PID");
    thread::set_working_dir(pid, path);

    trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, 0);
}

fn char_ptr_to_string(arr: *mut u8) -> String {
    let mut len = 0;
    while unsafe { *arr.add(len) } != 0 {
        len += 1;
    }
    let slice = unsafe { core::slice::from_raw_parts(arr, len) };
    String::from_utf8(slice.to_vec()).expect("Failed to convert char ptr to string")
}

fn running_thread_fd_lookup(fd: usize) -> usize {
    let pid = thread::get_running_thread_id().expect("Failed to get running thread PID");
    thread::get_fd(pid, fd).expect("Failed to get file descriptor")
}

fn running_thread_full_path(path: String) -> String {
    let pid = thread::get_running_thread_id().expect("Failed to get running thread PID");
    let working_dir = thread::get_working_dir(pid).expect("Failed to get working directory");

    let full_path = format!("{}/{}", working_dir, path);
    let full_path = path_process(full_path.as_str());

    full_path
}

fn running_thread_new_fd(fd: usize) -> usize {
    let pid = thread::get_running_thread_id().expect("Failed to get running thread PID");
    thread::add_fd(pid, fd)
}