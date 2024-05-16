use core::arch::asm;
use core::arch::global_asm;
use core::hint;
use core::mem::size_of;

use aarch64_cpu::asm::barrier;
use aarch64_cpu::registers::Readable;
use aarch64_cpu::registers::Writeable;
use aarch64_cpu::registers::SCTLR_EL1::C;
use aarch64_cpu::registers::SP_EL0;
use alloc::boxed::Box;
use alloc::rc::Rc;
use cpu::cpu::disable_kernel_space_interrupt;
use cpu::cpu::enable_kernel_space_interrupt;
use cpu::cpu::run_user_code;
use cpu::thread::CPUContext;
use cpu::thread::Thread;
use library::console::console;
use library::console::ConsoleMode;
use library::println;
use library::sync::mutex::Mutex;

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

extern "C" {
    pub fn store_context(context: *mut CPUContext);
    pub fn load_context(context: *const CPUContext);
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
#[derive(Debug, Clone, Copy)]
pub struct StackInfo {
    pub top: *mut u8,
    pub bottom: *mut u8,
}

impl StackInfo {
    pub const fn new(top: *mut u8, bottom: *mut u8) -> Self {
        Self { top, bottom }
    }
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct Task {
    pub thread: Thread,
    state: TaskState,
    kernel_stack: StackInfo,
    user_stack: StackInfo,
    pid: Rc<Mutex<PID>>,
    signal_handlers: [Option<fn() -> !>; Signal::Max as usize],
    /// The signals that are pending for the task, represented as a bitfield.
    pending_signals: u32,
    /// Whether the task is currently handling a signal.
    /// This is used to prevent signal handlers from being reentrant.
    doing_signal: bool,
    /// The context that was saved when the task do a user defined signal handler.
    signal_saved_context: Option<(u64, CPUContext)>,
}

impl Task {
    pub const USER_STACK_SIZE: usize = PAGE_SIZE * 4;

    pub fn new(stack: StackInfo) -> Self {
        Self {
            thread: Thread::new(),
            state: TaskState::Running,
            kernel_stack: stack,
            user_stack: StackInfo::new(core::ptr::null_mut(), core::ptr::null_mut()),
            pid: pid_manager().new_pid(),
            signal_handlers: [None; Signal::Max as usize],
            pending_signals: 0,
            doing_signal: false,
            signal_saved_context: None,
        }
    }

    pub fn from_job(job: fn() -> !) -> Self {
        // call into_raw to prevent the Box from being dropped
        let mut stack = Box::into_raw(Box::new([0_u8; 1024 * PAGE_SIZE]));
        let stack_bottom = (stack as usize + (unsafe { *stack }).len()) as *mut u8;
        let mut task = Self::new(StackInfo::new(stack as *mut u8, stack_bottom));
        task.thread.context.pc = job as u64;
        task.thread.context.sp = stack_bottom as u64;
        task
    }

    /// Fork a new task from the current task and return the child task pointer
    /// This function should never be inlined because the stack is copied from the parent task to the child task
    #[inline(never)]
    pub fn fork(&self, caller_sp: usize) -> *mut Task {
        // save return address
        let mut return_addr: usize = 0;
        unsafe { asm!("mov {}, lr", out(reg) return_addr) };

        let has_user_stack = self.user_stack.top != core::ptr::null_mut();
        // allocate a new stack for the child task
        let mut kernel_stack = Box::into_raw(Box::new([0_u8; 1024 * PAGE_SIZE]));
        let kernel_stack_bottom =
            (kernel_stack as usize + (unsafe { *kernel_stack }).len()) as *mut u8;
        let kernel_used_stack_len = self.kernel_stack.bottom as u64 - caller_sp as u64;
        // copy the stack from the parent task to the child task
        unsafe {
            core::ptr::copy_nonoverlapping(
                self.kernel_stack.bottom.sub(kernel_used_stack_len as usize),
                kernel_stack_bottom.sub(kernel_used_stack_len as usize),
                kernel_used_stack_len as usize,
            );
        }
        // create a child task
        let mut task = Self::new(StackInfo::new(kernel_stack as *mut u8, kernel_stack_bottom));
        if has_user_stack {
            let mut user_stack = Box::into_raw(Box::new([0_u8; Self::USER_STACK_SIZE]));
            let user_stack_bottom =
                (user_stack as usize + (unsafe { *user_stack }).len()) as *mut u8;
            task.user_stack = StackInfo::new(user_stack as *mut u8, user_stack_bottom);
            let user_used_stack_len = self.user_stack.bottom as u64 - SP_EL0.get();
            unsafe {
                core::ptr::copy_nonoverlapping(
                    self.user_stack.bottom.sub(user_used_stack_len as usize),
                    task.user_stack.bottom.sub(user_used_stack_len as usize),
                    user_used_stack_len as usize,
                );
            }
            task.thread.sp_el0 = user_stack_bottom as u64 - user_used_stack_len;
        };
        // copy the current thread context
        // the registers are stored in the stack in compiler generated function prologue
        unsafe {
            asm!("mov x0, {}
            ldp {}, {}, [x0, -16]
            ldp {}, {}, [x0, -32]
            ldp {}, {}, [x0, -48]
            ldp {}, {}, [x0, -64]
            ldp {}, {}, [x0, -80]
            ldr {}, [x0, -88]", in(reg) caller_sp, out(reg) task.thread.context.x20, out(reg) task.thread.context.x19, out(reg) task.thread.context.x22, out(reg) task.thread.context.x21, out(reg) task.thread.context.x24, out(reg) task.thread.context.x23, out(reg) task.thread.context.x26, out(reg) task.thread.context.x25, out(reg) task.thread.context.x28, out(reg) task.thread.context.x27, out(reg) task.thread.context.fp);
        }
        // child thread will jump to child_entry function and use ret to return to the caller
        task.thread.context.pc = child_entry as u64;
        // set the new kernel stack pointer
        // the new kernel stack
        task.thread.context.sp = kernel_stack_bottom as u64 - kernel_used_stack_len;
        unsafe {
            // place the caller sp at sp - 8
            core::ptr::write(
                (task.kernel_stack.bottom.sub(kernel_used_stack_len as usize) as *mut usize).sub(1),
                task.kernel_stack.bottom as usize - kernel_used_stack_len as usize,
            );
            // place the return address at sp - 16
            core::ptr::write(
                (task.kernel_stack.bottom.sub(kernel_used_stack_len as usize) as *mut usize).sub(2),
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

    #[inline(always)]
    pub fn stack(&self) -> StackInfo {
        self.kernel_stack
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

    pub fn run_user_program(&mut self, user_program: *const fn() -> !) {
        let code_start = user_program as u64;
        let stack_top = Box::into_raw(Box::new([0_u8; Self::USER_STACK_SIZE])) as u64;
        let stack_bottom = stack_top + Self::USER_STACK_SIZE as u64;
        self.user_stack.top = stack_top as *mut u8;
        self.user_stack.bottom = stack_bottom as *mut u8;
        unsafe { run_user_code(stack_bottom, code_start) };
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
                    let mut signal_stack = Box::into_raw(Box::new([0_u8; Self::USER_STACK_SIZE]));
                    let signal_stack_end = signal_stack as u64 + Self::USER_STACK_SIZE as u64;
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
                            unsafe { enable_kernel_space_interrupt() }
                            run_user_code(
                                (signal_stack_end as usize - (2 * size_of::<fn() -> !>())) as u64,
                                signal_hander_wrapper as u64,
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

    pub fn load_signal_context(&mut self) {
        let (sp, context) = self.signal_saved_context.take().unwrap();
        SP_EL0.set(sp);
        unsafe { load_context(&context as *const CPUContext) };
    }
}

#[inline(always)]
fn child_entry() {
    unsafe {
        asm!(
            "ldr x0, [sp, -8]
    ldr lr, [sp, -16]
    mov sp, x0
    mov x0, {}", in(reg) current() as usize
        )
    }
}

// prevent the function change the stack pointer
#[inline(always)]
fn signal_hander_wrapper() -> ! {
    unsafe {
        asm!(
            "ldr x0, [sp]
        blr x0
        mov x8, 10
        svc 0"
        )
    };
    unreachable!();
}
