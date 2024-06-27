use crate::alloc::string::ToString;
use crate::interrupt::timer::add_timer;
use crate::println_polling;
use alloc::string::String;
use alloc::vec::Vec;
use crate::fs::vfs::File;

use crate::uart;

use core::arch::asm;

use crate::println;
use crate::PAGE_ALLOC;

use crate::timer;

#[derive(PartialEq, Debug)]
pub enum IOType {
    UartRead,
}
#[derive(Debug)]
pub enum ProcessState {
    Ready,
    Running,
    Waiting(IOType),
    Terminated,
}
#[derive(Debug)]
struct ProcessMemoryLayout {
    text: MemorySection,
    data: MemorySection,
    stack: MemorySection,
}

#[derive(Debug)]
struct MemorySection {
    start: usize,
    size: usize,
}

#[repr(C, packed)]
#[derive(Clone, Copy, Debug)]
pub struct Registers {
    pub x0: u64,
    pub x1: u64,
    pub x2: u64,
    pub x3: u64,
    pub x4: u64,
    pub x5: u64,
    pub x6: u64,
    pub x7: u64,
    pub x8: u64,
    pub x9: u64,
    pub x10: u64,
    pub x11: u64,
    pub x12: u64,
    pub x13: u64,
    pub x14: u64,
    pub x15: u64,
    pub x16: u64,
    pub x17: u64,
    pub x18: u64,
    pub x19: u64,
    pub x20: u64,
    pub x21: u64,
    pub x22: u64,
    pub x23: u64,
    pub x24: u64,
    pub x25: u64,
    pub x26: u64,
    pub x27: u64,
    pub x28: u64,
    pub x29: u64,
    pub x30: u64,
}

impl Registers {
    pub fn new() -> Self {
        Self {
            x0: 0,
            x1: 0,
            x2: 0,
            x3: 0,
            x4: 0,
            x5: 0,
            x6: 0,
            x7: 0,
            x8: 0,
            x9: 0,
            x10: 0,
            x11: 0,
            x12: 0,
            x13: 0,
            x14: 0,
            x15: 0,
            x16: 0,
            x17: 0,
            x18: 0,
            x19: 0,
            x20: 0,
            x21: 0,
            x22: 0,
            x23: 0,
            x24: 0,
            x25: 0,
            x26: 0,
            x27: 0,
            x28: 0,
            x29: 0,
            x30: 0,
        }
    }
}
pub struct Process {
    pub pid: usize,
    pub state: ProcessState,
    pub registers: Registers,
    pub sp_el0: usize,
    proc_map: ProcessMemoryLayout,
    pub elr_el1: u64,
    pub current_dir: String,
    pub open_files: Vec<File>,
}

impl Process {
    fn new(pid: usize, code: &[u8]) -> Self {
        // alloc some page for the process
        let page_alloc = unsafe { &mut PAGE_ALLOC };
        const PAGE_SIZE: usize = 0x1000;
        // calculate how many pages we need to store text
        let text_num_pages = (code.len() + 0xfff) / 0x1000;
        let proc_mem_start = page_alloc.page_alloc(text_num_pages + 8) as usize;

        let proc_map = ProcessMemoryLayout {
            text: MemorySection {
                start: proc_mem_start,
                size: text_num_pages * PAGE_SIZE,
            },
            data: MemorySection {
                start: proc_mem_start as usize + text_num_pages * PAGE_SIZE,
                size: PAGE_SIZE * 4,
            },
            stack: MemorySection {
                start: proc_mem_start as usize + text_num_pages * PAGE_SIZE + PAGE_SIZE * 4,
                size: PAGE_SIZE * 4,
            },
        };
        // copy code to the text section
        unsafe {
            core::ptr::copy(code.as_ptr(), proc_map.text.start as *mut u8, code.len());
        }
        let mut open_files = Vec::new();
        let vfs = unsafe {crate::GLOBAL_VFS.as_mut().unwrap()};
        for _ in 0..2{
            let f = vfs.open("/dev/uart", false);
            match f {
                Ok(f) => {
                    open_files.push(f);
                }
                Err(e) => {
                    println_polling!("no devfs: {}", e);
                }
            }

        }


        Self {
            pid,
            state: ProcessState::Ready,
            registers: Registers::new(),
            sp_el0: proc_map.stack.start + proc_map.stack.size,
            proc_map,
            elr_el1: proc_mem_start as u64,
            current_dir: "/".to_string(),
            open_files: open_files,
        }
    }

    fn fork(&self, child_pid: usize) -> Self {
        let page_alloc = unsafe { &mut PAGE_ALLOC };
        const PAGE_SIZE: usize = 0x1000;
        // calculate how many pages we need to store text
        let new_mem = page_alloc.page_alloc(16) as usize;
        let parent_map = &self.proc_map;
        let proc_map = ProcessMemoryLayout {
            // text section is shared between parent and child
            text: MemorySection {
                start: parent_map.text.start,
                size: self.proc_map.text.size,
            },
            data: MemorySection {
                start: new_mem,
                size: PAGE_SIZE * 4,
            },
            stack: MemorySection {
                start: new_mem + PAGE_SIZE * 4,
                size: PAGE_SIZE * 4,
            },
        };
        // println_polling!("parent memory layout: {:?}", parent_map);
        // println_polling!("child memory layout: {:?}", proc_map);

        // copy data section
        unsafe {
            core::ptr::copy(
                parent_map.data.start as *const u8,
                proc_map.data.start as *mut u8,
                parent_map.data.size,
            );
        }
        // copy stack section
        unsafe {
            core::ptr::copy(
                parent_map.stack.start as *const u8,
                proc_map.stack.start as *mut u8,
                parent_map.stack.size,
            );
        }

        let mut child_registers = self.registers.clone();
        child_registers.x0 = 0;
        let new_sp = self.sp_el0 - parent_map.stack.start + proc_map.stack.start;
        // println_polling!("old sp: {:#x}", self.sp_el0);
        // println_polling!("new sp: {:#x}", new_sp);
        Self {
            pid: child_pid,
            state: ProcessState::Ready,
            registers: child_registers,
            sp_el0: new_sp,
            proc_map,
            elr_el1: self.elr_el1,
            current_dir: self.current_dir.clone(),
            open_files: Vec::new(),
        }
    }

    pub fn do_uart_read(&mut self) {
        let mut buf: *mut u8 = self.registers.x0 as *mut u8;
        let buf_len: usize = self.registers.x1 as usize;
        let buf_end = unsafe { buf.add(buf_len) };
        let mut read_len: usize = 0;
        while let Some(u) = uart::pop_read_buf() {
            if buf < buf_end {
                unsafe {
                    *buf = u;
                    buf = buf.add(1);
                }
                read_len += 1;
            } else {
                break;
            }
        }
        // println_polling!("read_len: {}", read_len);
        self.registers.x0 = read_len as u64;
        self.state = ProcessState::Ready;
    }

    pub fn exec(&mut self, code: &[u8]) {
        // copy code to the text section
        println_polling!("copy code to text section");
        unsafe {
            core::ptr::copy(
                code.as_ptr(),
                self.proc_map.text.start as *mut u8,
                code.len(),
            );
        }
        self.sp_el0 = self.proc_map.stack.start + self.proc_map.stack.size;
        self.elr_el1 = self.proc_map.text.start as u64;
        self.state = ProcessState::Ready;
    }
}

pub struct ProcessScheduler {
    pub processes: Vec<Process>, // pid to process
    current_pid: usize,
    is_running: bool,
    pub ksp: *mut u64,
}

#[no_mangle]
#[inline(never)]
pub fn proc_boink() {
    unsafe { asm!("nop") };
}

pub fn get_elr_el1() -> u64 {
    let elr_el1: u64;
    unsafe {
        asm!("mrs {0}, elr_el1", out(reg) elr_el1);
    }
    elr_el1
}

pub fn set_elr_el1(addr: u64) {
    unsafe {
        asm!("msr elr_el1, {0}", in(reg) addr);
    }
}

pub fn set_sp_el0(sp: u64) {
    unsafe {
        asm!("msr sp_el0, {0}", in(reg) sp);
    }
}

pub fn get_sp_el0() -> u64 {
    let sp_el0: u64;
    unsafe {
        asm!("mrs {0}, sp_el0", out(reg) sp_el0);
    }
    sp_el0
}

pub fn context_switch(message: String, sp: *mut u64) {
    // println_polling!("context switch");
    let process_scheduler = unsafe { crate::PROCESS_SCHEDULER.as_mut().unwrap() };
    let (cur_pid, cur_process) = process_scheduler.save_current_process(ProcessState::Ready, sp);

    // find the next ready process
    let next_pid = process_scheduler.get_next_ready(process_scheduler.get_current_pid());

    // run the next process
    process_scheduler.set_next_process(next_pid);

    add_timer(context_switch, 30, "".to_string());
}

impl ProcessScheduler {
    pub fn new() -> Self {
        Self {
            processes: Vec::new(),
            current_pid: 0,
            is_running: false,
            ksp: core::ptr::null_mut(),
        }
    }
    pub fn create_process(&mut self, code: &[u8]) -> usize {
        let pid = self.processes.len();
        let process = Process::new(pid, code);
        self.processes.push(process);
        pid
    }

    pub fn start(&mut self) {
        // run current process
        println!("Start running process {}", self.current_pid);
        let current_process = self.processes.get_mut(self.current_pid).unwrap();
        let sp_el0 = current_process.sp_el0;
        let elr_el1 = current_process.proc_map.text.start;
        self.is_running = true;
        // set timer
        timer::add_timer(context_switch, 30, "".to_string());
        unsafe {
            core::arch::asm!("
                msr spsr_el1, {k}
                msr elr_el1, {a}
                msr sp_el0, {s}
                eret
                ", k = in(reg) (0x3c0 ^ (0b1 << 7)) as u64, a = in(reg) elr_el1 as u64, s = in(reg) sp_el0 as u64);
        }
    }

    // fork current process
    pub fn fork(&mut self) -> usize {
        let new_pid = self.processes.len();
        let process = self.processes.get_mut(self.current_pid).unwrap();

        let new_process = process.fork(new_pid);
        self.processes.push(new_process);
        // println_polling!("Forked process {} from process {}", new_pid, self.current_pid);
        new_pid
    }

    pub fn is_running(&self) -> bool {
        self.is_running
    }

    pub fn get_current_pid(&self) -> usize {
        self.current_pid
    }

    pub fn get_process(&mut self, pid: usize) -> Option<&mut Process> {
        self.processes.get_mut(pid)
    }

    pub fn save_current_process(
        &mut self,
        state: ProcessState,
        sp: *mut u64,
    ) -> (usize, &mut Process) {
        let current_pid = self.get_current_pid();
        let registers = sp as *mut Registers;
        let current_process = self.get_process(current_pid).unwrap();
        current_process.registers = unsafe { *registers };
        current_process.elr_el1 = get_elr_el1();
        current_process.sp_el0 = get_sp_el0() as usize;
        current_process.state = state;
        (current_pid, current_process)
    }

    pub fn set_next_process(&mut self, next_pid: usize) {
        // println_polling!("Switch to process {}", next_pid);
        for i in 0..self.processes.len() {
            // println_polling!("process {}: {:?}", i, self.processes[i].state);
        }
        self.current_pid = next_pid;
        let registers = self.ksp as *mut Registers;
        let next_process = self.get_process(next_pid).unwrap();
        next_process.state = ProcessState::Running;
        // set the next process to current process
        // set the registers
        let next_registers = &next_process.registers;
        let next_sp = next_process.sp_el0;
        let next_elr_el1 = next_process.elr_el1;
        unsafe { *registers = *next_registers };
        // println_polling!("next regs: {:?}", next_registers);
        // println_polling!("next sp: {:#x}", next_sp);
        // println_polling!("next elr_el1: {:#x}", next_elr_el1);
        set_elr_el1(next_elr_el1);
        set_sp_el0(next_sp as u64);
    }

    // pub fn skip_current_process(&mut self) {
    //     let current_pid = self.get_current_pid();
    //     let next_pid = self.get_next_ready(current_pid);
    //     self.set_next_process(next_pid);
    // }

    pub fn next_process(&mut self) {
        let current_pid = self.get_current_pid();
        let next_pid = self.get_next_ready(current_pid);
        self.set_next_process(next_pid);
    }

    pub fn get_waiting_process(&mut self, io_type: IOType) -> Option<&mut Process> {
        for (_pid, process) in self.processes.iter_mut().enumerate() {
            match &process.state {
                ProcessState::Waiting(t) => {
                    if *t == io_type {
                        return Some(process);
                    }
                }
                _ => {}
            }
        }
        None
    }

    pub fn get_next_ready(&mut self, cur_pid: usize) -> usize {
        let mut next_pid = (cur_pid + 1) % self.processes.len();
        loop {
            match self.processes[next_pid].state {
                ProcessState::Ready => break,
                _ => {}
            }
            next_pid = (next_pid + 1) % self.processes.len();
        }
        next_pid
    }

    pub fn terminate(&mut self, pid: usize) {
        self.processes[pid].state = ProcessState::Terminated;
        if self.current_pid == pid {
            self.next_process();
        }
    }
}
