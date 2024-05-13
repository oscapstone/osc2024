use crate::os::stdio::{print_hex_now, println_now};

use super::exception_handler::trap_frame;
use alloc::collections::BTreeSet;
use core::arch::asm;

static mut THREADS: Option<BTreeSet<Thread>> = None;
pub static mut TRAP_FRAME_PTR: Option<*mut u64> = None;

#[derive(Clone)]
struct ThreadContext {
    x0: usize,
    x1: usize,
    x2: usize,
    x3: usize,
    x4: usize,
    x5: usize,
    x6: usize,
    x7: usize,
    x8: usize,
    x9: usize,
    x10: usize,
    x11: usize,
    x12: usize,
    x13: usize,
    x14: usize,
    x15: usize,
    x16: usize,
    x17: usize,
    x18: usize,
    x19: usize,
    x20: usize,
    x21: usize,
    x22: usize,
    x23: usize,
    x24: usize,
    x25: usize,
    x26: usize,
    x27: usize,
    x28: usize,
    x29: usize,
    x30: usize,
    x31: usize,
    pc: usize,
    sp: usize,
}

impl Default for ThreadContext {
    fn default() -> Self {
        ThreadContext {
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
            x31: 0,
            pc: 0,
            sp: 0,
        }
    }
}

#[derive(Clone)]
enum ThreadState {
    Running(ThreadContext),
    Waiting(ThreadContext),
    Dead,
}

#[derive(Clone)]
struct Thread {
    id: usize,
    program: *mut u8,
    program_size: usize,
    stack: *mut u8,
    stack_size: usize,
    state: ThreadState,
}

impl Default for Thread {
    fn default() -> Self {
        Thread {
            id: 0,
            program: core::ptr::null_mut(),
            program_size: 0,
            stack: core::ptr::null_mut(),
            stack_size: 0,
            state: ThreadState::Dead,
        }
    }
}

impl Ord for Thread {
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        self.id.cmp(&other.id)
    }
}

impl PartialOrd for Thread {
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl PartialEq for Thread {
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
    }
}

impl Eq for Thread {}

pub fn init() {
    unsafe {
        THREADS = Some(BTreeSet::new());
    }
}

pub fn create_thread(
    program: *mut u8,
    program_size: usize,
    stack: *mut u8,
    stack_size: usize,
) -> usize {
    unsafe {
        let threads = THREADS.as_mut().unwrap();
        let id = threads.len();
        threads.insert(Thread {
            id,
            program,
            program_size,
            stack,
            stack_size,
            state: ThreadState::Waiting(ThreadContext {
                pc: program as usize,
                sp: stack as usize + program_size,
                ..Default::default()
            }),
        });
        println!("Stack: {:X?}", stack as usize + stack_size);
        println!("Thread created: {}", id);
        id
    }
}

pub fn run_thread(id: usize) {
    unsafe {
        let threads = THREADS.as_mut().unwrap();
        let mut target_thread = match threads.get(&Thread {
            id,
            ..Default::default()
        }) {
            Some(thread) => thread.clone(),
            None => {
                println!("Thread not found");
                return;
            }
        };

        target_thread.state = match target_thread.state {
            ThreadState::Waiting(context) => ThreadState::Running(context),
            _ => {
                println!("Thread is not in waiting state");
                return;
            }
        };

        threads.remove(&Thread {
            id,
            ..Default::default()
        });
        threads.insert(target_thread.clone());

        if let ThreadState::Running(context) = &target_thread.state {
            asm!(
                "mov {tmp}, 0x200",
                "msr spsr_el1, {tmp}",
                "mov {tmp}, {pc}",
                "msr elr_el1, {tmp}",
                "mov {tmp}, {sp}",
                "msr sp_el0, {tmp}",
                "eret",
                pc = in(reg) context.pc,
                sp = in(reg) context.sp,
                tmp = out(reg) _,
            );
        }
    }
}

pub fn get_id_by_pc(pc: usize) -> Option<usize> {
    unsafe {
        let threads = THREADS.as_ref().unwrap();
        for thread in threads.iter() {
            if let ThreadState::Running(context) = &thread.state {
                if (thread.program as usize) <= pc
                    && pc < (thread.program as usize + thread.program_size)
                {
                    return Some(thread.id);
                }
            }
        }
        None
    }
}

fn save_context(trap_frame_ptr: *mut u64) -> bool {
    unsafe {
        let threads = THREADS.as_mut().unwrap();
        let mut current_thread = None;

        for thread in threads.iter() {
            if let ThreadState::Running(context) = &thread.state {
                current_thread = Some(thread.clone());
                let current_pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC) as usize;
                if !((current_thread.as_ref().unwrap()).program as usize <= current_pc
                    && current_pc
                        < current_thread.as_ref().unwrap().program as usize
                            + current_thread.as_ref().unwrap().program_size)
                {
                    current_thread = None;
                }
            }
        }

        if current_thread.is_some() {
            threads.remove(current_thread.as_ref().unwrap());

            if let ThreadState::Running(ref mut context) = current_thread.as_mut().unwrap().state {
                context.x0 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X0) as usize;
                context.x1 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X1) as usize;
                context.x2 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X2) as usize;
                context.x3 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X3) as usize;
                context.x4 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X4) as usize;
                context.x5 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X5) as usize;
                context.x6 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X6) as usize;
                context.x7 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X7) as usize;
                context.x8 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X8) as usize;
                context.x9 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X9) as usize;
                context.x10 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X10) as usize;
                context.x11 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X11) as usize;
                context.x12 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X12) as usize;
                context.x13 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X13) as usize;
                context.x14 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X14) as usize;
                context.x15 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X15) as usize;
                context.x16 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X16) as usize;
                context.x17 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X17) as usize;
                context.x18 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X18) as usize;
                context.x19 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X19) as usize;
                context.x20 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X20) as usize;
                context.x21 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X21) as usize;
                context.x22 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X22) as usize;
                context.x23 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X23) as usize;
                context.x24 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X24) as usize;
                context.x25 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X25) as usize;
                context.x26 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X26) as usize;
                context.x27 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X27) as usize;
                context.x28 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X28) as usize;
                context.x29 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X29) as usize;
                context.x30 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X30) as usize;
                context.x31 = trap_frame::get(trap_frame_ptr, trap_frame::Register::X31) as usize;
                context.pc = trap_frame::get(trap_frame_ptr, trap_frame::Register::PC) as usize;
                context.sp = trap_frame::get(trap_frame_ptr, trap_frame::Register::SP) as usize;
            }
            threads.insert(current_thread.unwrap());
            true
        } else {
            false
        }
    }
}

fn restore_context(trap_frame_ptr: *mut u64) {
    let threads = unsafe { THREADS.as_mut().unwrap() };
    let mut current_thread = threads.iter().next().unwrap().clone();

    for thread in threads.iter() {
        if let ThreadState::Running(context) = &thread.state {
            current_thread = thread.clone();
            break;
        }
    }

    if let ThreadState::Running(ref context) = current_thread.state {
        unsafe {
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X0, context.x0 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X1, context.x1 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X2, context.x2 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X3, context.x3 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X4, context.x4 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X5, context.x5 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X6, context.x6 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X7, context.x7 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X8, context.x8 as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::X9, context.x9 as u64);
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X10,
                context.x10 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X11,
                context.x11 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X12,
                context.x12 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X13,
                context.x13 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X14,
                context.x14 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X15,
                context.x15 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X16,
                context.x16 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X17,
                context.x17 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X18,
                context.x18 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X19,
                context.x19 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X20,
                context.x20 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X21,
                context.x21 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X22,
                context.x22 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X23,
                context.x23 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X24,
                context.x24 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X25,
                context.x25 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X26,
                context.x26 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X27,
                context.x27 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X28,
                context.x28 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X29,
                context.x29 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X30,
                context.x30 as u64,
            );
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::X31,
                context.x31 as u64,
            );
            trap_frame::set(trap_frame_ptr, trap_frame::Register::PC, context.pc as u64);
            trap_frame::set(trap_frame_ptr, trap_frame::Register::SP, context.sp as u64);
        }
    }
}

#[no_mangle]
pub fn context_switching() {
    unsafe {
        let threads = THREADS.as_mut().unwrap();
        let trap_frame_ptr = TRAP_FRAME_PTR.unwrap();

        if !save_context(trap_frame_ptr) {
            return;
        }

        let mut current_thread_iter = threads.iter();
        let mut next_thread;

        loop {
            let mut current_thread = match current_thread_iter.next() {
                Some(thread) => thread.clone(),
                None => {
                    next_thread = threads.iter().next().unwrap().clone();
                    break;
                }
            };

            if let ThreadState::Running(_) = current_thread.state {
                // println!("Current thread: {}", current_thread.id);
                // println!(
                //     "Current thread state: {:?}",
                //     match current_thread.state {
                //         ThreadState::Running(_) => "Running",
                //         ThreadState::Waiting(_) => "Waiting",
                //         ThreadState::Dead => "Dead",
                //     }
                // );
                // println!(
                //     "Current thread PC: {:X?}",
                //     match &current_thread.state {
                //         ThreadState::Running(context) => context.pc,
                //         ThreadState::Waiting(context) => context.pc,
                //         ThreadState::Dead => 0,
                //     }
                // );
                next_thread = match current_thread_iter.next() {
                    Some(thread) => thread.clone(),
                    None => threads.iter().next().unwrap().clone(),
                };
                assert!(threads.remove(&current_thread));
                current_thread.state = match current_thread.state {
                    ThreadState::Running(context) => ThreadState::Waiting(context),
                    _ => ThreadState::Dead,
                };
                assert!(threads.insert(current_thread));
                break;
            }
        }

        // println!("Next thread: {}", next_thread.id);
        // println!(
        //     "Next thread state: {:?}",
        //     match next_thread.state {
        //         ThreadState::Running(_) => "Running",
        //         ThreadState::Waiting(_) => "Waiting",
        //         ThreadState::Dead => "Dead",
        //     }
        // );
        // println!(
        //     "Next thread PC: {:X?}",
        //     match &next_thread.state {
        //         ThreadState::Running(context) => context.pc,
        //         ThreadState::Waiting(context) => context.pc,
        //         ThreadState::Dead => 0,
        //     }
        // );

        assert!(threads.remove(&next_thread));
        next_thread.state = match next_thread.state {
            ThreadState::Waiting(context) => ThreadState::Running(context),
            ThreadState::Running(context) => ThreadState::Running(context),
            _ => ThreadState::Dead,
        };
        assert!(threads.insert(next_thread));

        restore_context(trap_frame_ptr);

        // println!("Context switched");
        // println!(
        //     "{:X?}",
        //     trap_frame::get(trap_frame_ptr, trap_frame::Register::PC)
        // );
    }
}
