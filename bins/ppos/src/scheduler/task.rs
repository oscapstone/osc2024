use core::arch::asm;
use core::arch::global_asm;
use core::mem::size_of;

use aarch64_cpu::registers::Readable;
use aarch64_cpu::registers::Writeable;
use aarch64_cpu::registers::SP_EL0;
use alloc::rc::Rc;
use alloc::vec;
use alloc::vec::Vec;
use bsp::memory::GPU_MEMORY_MMIO_BASE;
use bsp::memory::GPU_MEMORY_MMIO_SIZE;
use cpu::cpu::disable_kernel_space_interrupt;
use cpu::cpu::enable_kernel_space_interrupt;
use cpu::cpu::run_user_code;
use cpu::thread::CPUContext;
use cpu::thread::Thread;
use library::sync::mutex::Mutex;

use crate::memory::paging::memory_mapping::MemoryExecutePermission;
use crate::memory::paging::memory_mapping::MemoryMapping;
use crate::memory::paging::page::MemoryAccessPermission;
use crate::memory::paging::page::MemoryAttribute;
use crate::memory::phys_to_virt;
use crate::memory::round_down;
use crate::memory::round_up;
use crate::memory::virt_to_phys;
use crate::memory::AllocatedMemory;
use crate::pid::pid_manager;
use crate::signal;
use crate::signal::Signal;
use crate::{
    memory::PAGE_SIZE,
    pid::{PIDNumber, PID},
    scheduler::current,
};

use super::scheduler;

global_asm!(include_str!("store_context.s"));
global_asm!(include_str!("load_context.s"));
global_asm!(include_str!("signal_handler_wrapper.s"));

extern "C" {
    pub fn store_context(context: *mut CPUContext);
    pub fn load_context(context: *const CPUContext);
    fn signal_handler_wrapper();
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum TaskState {
    Running,
    Interruptable,
    Uninterruptable,
    Zombie,
    Stopped,
    Traced,
    Dead,
    Swapping,
}

#[repr(C)]
#[derive(Debug)]
pub struct Task {
    pub thread: Thread,
    state: TaskState,
    kernel_stack: AllocatedMemory,
    /// The user stack infomation.
    /// The field is used to recover the user stack when the task is back from the signal handler.
    /// It will be None when the task is not handling a signal.
    user_stack: Option<Rc<AllocatedMemory>>,
    pid: Rc<Mutex<PID>>,
    signal_handlers: [Option<fn() -> !>; Signal::Max as usize],
    /// The signals that are pending for the task, represented as a bitfield.
    pending_signals: u32,
    /// Whether the task is currently handling a signal.
    /// This is used to prevent signal handlers from being reentrant.
    doing_signal: bool,
    /// The context that was saved when the task do a user defined signal handler.
    signal_saved_context: Option<(u64, CPUContext)>,
    /// user space page table
    memory_mapping: Rc<MemoryMapping>,
}

impl Task {
    pub const USER_STACK_SIZE: usize = PAGE_SIZE * 4;
    pub const USER_STACK_START: usize = 0xffff_ffff_b000;
    pub const USER_STACK_END: usize = Self::USER_STACK_START + Self::USER_STACK_SIZE;
    pub const KERNEL_STACK_SIZE: usize = PAGE_SIZE * 1024;
    const SIGNAL_HANDLER_WRAPPER_SHARE_START: usize = Self::USER_STACK_START - PAGE_SIZE;

    pub fn new(stack: AllocatedMemory, memory_mapping: Option<Rc<MemoryMapping>>) -> Self {
        Self {
            thread: Thread::new(),
            state: TaskState::Running,
            kernel_stack: stack,
            user_stack: None,
            pid: pid_manager().new_pid(),
            signal_handlers: [None; Signal::Max as usize],
            pending_signals: 0,
            doing_signal: false,
            signal_saved_context: None,
            memory_mapping: match memory_mapping {
                Some(memory_mapping) => memory_mapping,
                None => Rc::new(MemoryMapping::new()),
            },
        }
    }

    pub fn from_job(job: fn() -> !) -> Self {
        let stack = AllocatedMemory::new(vec![0_u8; Self::KERNEL_STACK_SIZE].into_boxed_slice());
        let stack_bottom = stack.bottom();
        let mut task = Self::new(stack, None);
        task.thread.context.pc = job as u64;
        task.thread.context.sp = stack_bottom as u64;
        task
    }

    /// Fork a new task from the current task and return the child task pointer
    /// This function should never be inlined because the stack is copied from the parent task to the child task
    #[inline(never)]
    pub fn fork(&self, caller_sp: usize) -> *mut Task {
        // save return address
        let mut return_addr: usize;
        unsafe { asm!("mov {}, lr", out(reg) return_addr) };

        // allocate a new stack for the child task
        let kernel_stack =
            AllocatedMemory::new(vec![0_u8; Self::KERNEL_STACK_SIZE].into_boxed_slice());
        let kernel_stack_bottom = kernel_stack.bottom();
        let kernel_used_stack_len = self.kernel_stack.bottom() as u64 - caller_sp as u64;
        // copy the stack from the parent task to the child task
        unsafe {
            core::ptr::copy_nonoverlapping(
                self.kernel_stack
                    .bottom()
                    .sub(kernel_used_stack_len as usize),
                kernel_stack_bottom.sub(kernel_used_stack_len as usize) as *mut u8,
                kernel_used_stack_len as usize,
            );
        }
        // create a child task
        let mut task = Self::new(
            kernel_stack,
            Some(Rc::new(self.memory_mapping.copy().unwrap())),
        );
        task.thread.sp_el0 = SP_EL0.get();
        // copy the current thread context
        // the registers are stored in the stack in compiler generated function prologue
        unsafe {
            asm!("ldp {}, {}, [x0, -16]
            ldp {}, {}, [x0, -32]
            ldp {}, {}, [x0, -48]
            ldp {}, {}, [x0, -64]
            ldp {}, {}, [x0, -80]
            ldr {}, [x0, -88]", out(reg) task.thread.context.x20, out(reg) task.thread.context.x19, out(reg) task.thread.context.x22, out(reg) task.thread.context.x21, out(reg) task.thread.context.x24, out(reg) task.thread.context.x23, out(reg) task.thread.context.x26, out(reg) task.thread.context.x25, out(reg) task.thread.context.x28, out(reg) task.thread.context.x27, out(reg) task.thread.context.fp, in("x0") caller_sp);
        }
        // child thread will jump to child_entry function and use ret to return to the caller
        task.thread.context.pc = child_entry as u64;
        // set the new kernel stack pointer
        // the new kernel stack
        task.thread.context.sp = kernel_stack_bottom as u64 - kernel_used_stack_len;
        unsafe {
            // place the caller sp at sp - 8
            core::ptr::write(
                (kernel_stack_bottom.sub(kernel_used_stack_len as usize) as *mut usize).sub(1),
                kernel_stack_bottom as usize - kernel_used_stack_len as usize,
            );
            // place the return address at sp - 16
            core::ptr::write(
                (kernel_stack_bottom.sub(kernel_used_stack_len as usize) as *mut usize).sub(2),
                return_addr,
            );
        }
        task.signal_handlers = self.signal_handlers.clone();
        let task = Rc::new(Mutex::new(task));
        let task_ptr = &mut *task.lock().unwrap() as *mut Task;
        scheduler().add_task(task);
        task_ptr
    }

    #[inline(always)]
    pub fn state(&self) -> TaskState {
        self.state
    }

    /**
     * Leave the kernel thread and return to the scheduler
     */
    pub fn exit(&mut self, code: usize) -> ! {
        self.state = TaskState::Dead;
        // let the idle task to clean up the task
        // we can't clean up the task here because the task is still running
        scheduler().schedule();
        panic!("Unreachable!")
    }

    pub fn kill(pid: PIDNumber) {
        unsafe { disable_kernel_space_interrupt() }
        if let Some(pid) = pid_manager().get_pid(pid as usize) {
            pid.lock()
                .unwrap()
                .pid_task()
                .unwrap()
                .lock()
                .unwrap()
                .state = TaskState::Dead;
        }
        unsafe { enable_kernel_space_interrupt() }
    }

    #[inline(always)]
    pub fn pid_number(&self) -> PIDNumber {
        self.pid.lock().unwrap().number()
    }

    #[inline(always)]
    pub fn pid(&self) -> Rc<Mutex<PID>> {
        self.pid.clone()
    }

    pub fn run_user_program(&mut self, user_program: &[u8]) {
        let block_len = round_up(user_program.len());
        let mut code_vec = Vec::with_capacity(block_len);
        for byte in user_program {
            code_vec.push(*byte);
        }
        // fill the rest with 0
        // ensure the code is aligned to the page size
        code_vec.resize(block_len, 0);
        let code = Rc::new(AllocatedMemory::new(code_vec.into_boxed_slice()));
        let code_start = code.as_ptr();
        self.memory_mapping
            .map_pages(
                0,
                Some(virt_to_phys(code_start as usize)),
                block_len,
                MemoryAttribute::Normal,
                MemoryAccessPermission::ReadOnlyEL1EL0,
                MemoryExecutePermission::AllowUser,
                Some(code),
            )
            .unwrap();
        self.memory_mapping
            .map_pages(
                GPU_MEMORY_MMIO_BASE,
                Some(phys_to_virt(GPU_MEMORY_MMIO_BASE)),
                GPU_MEMORY_MMIO_SIZE,
                MemoryAttribute::Device,
                MemoryAccessPermission::ReadWriteEL1EL0,
                MemoryExecutePermission::Deny,
                None,
            )
            .unwrap();
        self.memory_mapping
            .map_pages(
                Self::USER_STACK_START,
                None,
                Self::USER_STACK_SIZE,
                MemoryAttribute::Normal,
                MemoryAccessPermission::ReadWriteEL1EL0,
                MemoryExecutePermission::Deny,
                None,
            )
            .unwrap();
        unsafe { run_user_code(Self::USER_STACK_END as u64, 0) };
    }

    pub fn set_signal_handler(&mut self, signal: Signal, handler: fn() -> !) {
        self.signal_handlers[signal as usize] = Some(handler);
    }

    pub fn send_signal(&mut self, signal: Signal) {
        self.pending_signals |= 1 << signal as u32;
    }

    pub fn do_pending_signal(&mut self) {
        unsafe { disable_kernel_space_interrupt() }
        if self.doing_signal {
            return;
        }
        self.doing_signal = true;
        for i in 0..Signal::Max as usize {
            if self.pending_signals & (1 << i) != 0 {
                if let Some(handler) = self.signal_handlers[i] {
                    unsafe { enable_kernel_space_interrupt() }
                    // create a new user stack
                    let signal_stack = Rc::new(AllocatedMemory::new(
                        vec![0_u8; Self::USER_STACK_SIZE].into_boxed_slice(),
                    ));
                    let signal_stack_start = signal_stack.as_ptr() as usize;
                    let signal_stack_end = signal_stack_start as u64 + Self::USER_STACK_SIZE as u64;
                    self.user_stack = match self
                        .memory_mapping
                        .get_region(Self::USER_STACK_START)
                        .unwrap()
                        .physical_memory()
                    {
                        Some(physical_memory) => Some(physical_memory),
                        None => panic!("User stack has not been allocated!"),
                    };
                    // save the current context
                    let mut context = CPUContext::new();
                    unsafe { disable_kernel_space_interrupt() }
                    unsafe { store_context(&mut context as *mut CPUContext) };
                    // the saved context resume from here
                    self.signal_saved_context = Some((SP_EL0.get(), context));
                    // check if the signal handler has been run
                    // use volatile read to prevent the compiler from optimizing the read
                    if unsafe { core::ptr::read_volatile(&self.pending_signals as *const u32) }
                        & (1 << i)
                        != 0
                    {
                        unsafe {
                            // write the handler to the stack
                            core::ptr::write(
                                (signal_stack_end as usize - (2 * size_of::<fn() -> !>()))
                                    as *mut fn() -> !,
                                handler,
                            );
                            // clear the pending signal
                            self.pending_signals &= !(1 << i);
                            // map the signal handler stack to the user space
                            self.memory_mapping
                                .unmap_pages(Self::USER_STACK_START, Self::USER_STACK_SIZE)
                                .unwrap();
                            self.memory_mapping
                                .map_pages(
                                    Self::USER_STACK_START,
                                    Some(virt_to_phys(signal_stack_start as usize)),
                                    Self::USER_STACK_SIZE,
                                    MemoryAttribute::Normal,
                                    MemoryAccessPermission::ReadWriteEL1EL0,
                                    MemoryExecutePermission::Deny,
                                    Some(signal_stack),
                                )
                                .unwrap();
                            // ensure the signal handler wrapper is accessible in the user space
                            self.memory_mapping
                                .map_pages(
                                    Self::SIGNAL_HANDLER_WRAPPER_SHARE_START,
                                    Some(round_down(virt_to_phys(signal_handler_wrapper as usize))),
                                    PAGE_SIZE,
                                    MemoryAttribute::Normal,
                                    MemoryAccessPermission::ReadOnlyEL1EL0,
                                    MemoryExecutePermission::AllowUser,
                                    None,
                                )
                                .unwrap();
                            enable_kernel_space_interrupt();
                            run_user_code(
                                (Self::USER_STACK_END - (2 * size_of::<fn() -> !>())) as u64,
                                (Self::SIGNAL_HANDLER_WRAPPER_SHARE_START
                                    | (signal_handler_wrapper as usize & 0xfff))
                                    as u64,
                            )
                        };
                    }
                    unsafe { enable_kernel_space_interrupt() }
                } else {
                    unsafe { enable_kernel_space_interrupt() }
                    match Signal::from(i) {
                        Signal::SIGHUP => todo!(),
                        Signal::SIGINT => todo!(),
                        Signal::SIGQUIT => todo!(),
                        Signal::SIGILL => todo!(),
                        Signal::SIGTRAP => todo!(),
                        Signal::SIGABRT => todo!(),
                        Signal::SIGFPE => todo!(),
                        Signal::SIGKILL => signal::default_kill_handler(),
                        Signal::SIGSEGV => todo!(),
                        Signal::SIGPIPE => todo!(),
                        Signal::SIGALRM => todo!(),
                        Signal::SIGTERM => todo!(),
                        _ => unreachable!(),
                    }
                }
            }
        }
        self.doing_signal = false;
    }

    pub fn back_from_signal(&mut self) {
        // unmap the signal stack
        // the signal stack will be recycled automatically
        self.memory_mapping
            .unmap_pages(Self::USER_STACK_START, Self::USER_STACK_SIZE)
            .unwrap();
        // map the user stack back
        self.memory_mapping
            .map_pages(
                Self::USER_STACK_START,
                Some(virt_to_phys(
                    self.user_stack.as_ref().unwrap().as_ptr() as usize
                )),
                Self::USER_STACK_SIZE,
                MemoryAttribute::Normal,
                MemoryAccessPermission::ReadWriteEL1EL0,
                MemoryExecutePermission::Deny,
                self.user_stack.clone(),
            )
            .unwrap();
        self.user_stack = None;
        // invalidate the signal handler wrapper share page
        self.memory_mapping
            .unmap_pages(Self::SIGNAL_HANDLER_WRAPPER_SHARE_START, PAGE_SIZE)
            .unwrap();
        // restore the context
        let (sp, context) = self.signal_saved_context.take().unwrap();
        SP_EL0.set(sp);
        unsafe { load_context(&context as *const CPUContext) };
    }

    #[inline(always)]
    pub fn memory_mapping(&self) -> Rc<MemoryMapping> {
        self.memory_mapping.clone()
    }
}

#[inline(never)]
unsafe fn child_entry() {
    asm!(
        "ldr x0, [sp, -8]
    ldr lr, [sp, -16]
    mov sp, x0
    mov x0, {}", in(reg) current() as usize
    )
}
