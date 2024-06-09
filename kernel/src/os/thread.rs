use super::{exception_handler::trap_frame, shell::INITRAMFS};
use crate::cpu::mmu::{self, vm_setup, vm_to_pm};
use crate::os::stdio::{print_hex_now, println_now};
use alloc::{alloc::dealloc, string::String, vec::Vec};
use core::ptr::{read_volatile, write_volatile};
use core::{alloc::Layout, arch::asm};

static mut THREADS: Option<Vec<Thread>> = None;
pub static mut TRAP_FRAME_PTR: Option<*mut u64> = None;

pub const THREAD_ALIGNMENT: usize = 4096;

#[derive(Clone, Default)]
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
    spsr: usize,
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
    page_table: *mut u8,
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
            page_table: core::ptr::null_mut(),
        }
    }
}

pub fn init() {
    unsafe {
        THREADS = Some(Vec::new());
    }
}

pub fn create_thread(
    program: *mut u8,
    program_size: usize,
    stack: *mut u8,
    stack_size: usize,
) -> usize {
    let threads = unsafe { THREADS.as_mut().unwrap() };
    let id = threads.len();

    let page_table = mmu::vm_setup(program, program_size, stack, stack_size);

    threads.push(Thread {
        id,
        program,
        program_size,
        stack,
        stack_size,
        state: ThreadState::Waiting(ThreadContext {
            pc: 0,
            sp: 0xFFFF_FFFF_FFF0,
            ..Default::default()
        }),
        page_table,
    });

    println!("Stack SP: {:X?}", stack as usize + stack_size);
    println!("Thread created: {}", id);
    id
}

#[no_mangle]
pub fn run_thread(id: Option<usize>) {
    unsafe {
        let threads = THREADS.as_mut().unwrap();

        let target_thread = match id {
            Some(id) => threads
                .iter_mut()
                .find(|thread| thread.id == id)
                .expect("Thread not found"),
            None => threads
                .iter_mut()
                .find(|thread| {
                    let state = &thread.state;
                    matches!(ThreadState::Waiting, state)
                })
                .expect("No running thread"),
        };

        target_thread.state = match &target_thread.state {
            ThreadState::Waiting(context) => ThreadState::Running(context.clone()),
            _ => {
                println!("Thread is not in waiting state");
                return;
            }
        };

        if let ThreadState::Running(context) = &target_thread.state {
            asm!(
                "msr ttbr0_el1, {PGD_addr}",
                PGD_addr = in(reg) target_thread.page_table
            );

            asm!(
                "mrs {tmp}, cntkctl_el1",
                "orr {tmp}, {tmp}, 1",
                "msr cntkctl_el1, {tmp}",
                tmp = out(reg) _,
            );

            assert_eq!(context.spsr, 0);

            asm!(
                "mov x0, {spsr}",
                "msr spsr_el1, x0",
                "mov x0, {pc}",
                "msr elr_el1, x0",
                "mov x0, {sp}",
                "msr sp_el0, x0",
                "eret",
                spsr = in(reg) context.spsr,
                pc = in(reg) context.pc,
                sp = in(reg) context.sp,
                options(noreturn),
            );
        }
    }
}

pub fn get_id_by_pc(pc: usize) -> Option<usize> {
    let threads = unsafe { THREADS.as_ref().unwrap() };

    threads
        .iter()
        .find(|thread| {
            if let ThreadState::Running(context) = &thread.state {
                true
                // (thread.program as usize) <= pc
                //     && pc < (thread.program as usize + thread.program_size)
            } else {
                false
            }
        })
        .map(|thread| thread.id)
}

pub fn save_context(trap_frame_ptr: *mut u64) -> bool {
    unsafe {
        let threads = THREADS.as_mut().unwrap();
        let current_thread = threads
            .iter_mut()
            .find(|thread| {
                let state = &thread.state;
                matches!(state, ThreadState::Running(_))
            })
            .expect("SAVE_CONTEXT: No running thread");

        current_thread.state =
            if let ThreadState::Running(mut context) = current_thread.state.clone() {
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
                context.spsr = trap_frame::get(trap_frame_ptr, trap_frame::Register::SPSR) as usize;

                ThreadState::Running(context)
            } else {
                panic!();
            };

        true
    }
}

pub fn restore_context(trap_frame_ptr: *mut u64) {
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
            trap_frame::set(
                trap_frame_ptr,
                trap_frame::Register::SPSR,
                context.spsr as u64,
            );
        }
    }
}

pub fn context_switching() {
    // println!("Context switching");

    let threads = unsafe { THREADS.as_mut().unwrap() };
    let trap_frame_ptr = unsafe { TRAP_FRAME_PTR.unwrap() };

    if !save_context(trap_frame_ptr) {
        println_now("CONTEXT_SWITCHING: Failed to save context");
        return;
    }

    // for i in threads.iter() {
    //     println!("PID: {}", i.id);
    //     println!(
    //         "state: {}",
    //         match i.state {
    //             ThreadState::Running(_) => "Running",
    //             ThreadState::Waiting(_) => "Waiting",
    //             ThreadState::Dead => "Dead",
    //         }
    //     );
    // }

    let mut current_thread_iter = threads.iter_mut();
    let mut current_thread;
    let mut next_thread;

    loop {
        current_thread = match current_thread_iter.next() {
            Some(thread) => thread,
            None => {
                break;
            }
        };

        if let ThreadState::Running(context) = &current_thread.state {
            current_thread.state = ThreadState::Waiting(context.clone());
            break;
        }
    }

    next_thread = current_thread_iter.find(|thread| {
        let state = &thread.state;
        matches!(state, ThreadState::Waiting(_))
    });

    if next_thread.is_none() {
        next_thread = threads.iter_mut().find(|thread| {
            let state = &thread.state;
            matches!(state, ThreadState::Waiting(_))
        });
        assert!(next_thread.is_some(), "No thread to switch");
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

    let next_thread = next_thread.unwrap();

    next_thread.state = match next_thread.state.clone() {
        ThreadState::Waiting(context) => ThreadState::Running(context),
        ThreadState::Running(context) => ThreadState::Running(context),
        _ => ThreadState::Dead,
    };

    assert!(matches!(next_thread.state, ThreadState::Running(_)));

    let next_thread_id = next_thread.id;

    unsafe {
        asm!(
            "dsb ish",
            "msr ttbr0_el1, {PGD_addr}",
            "tlbi vmalle1is",
            "dsb ish",
            "isb",
            PGD_addr = in(reg) next_thread.page_table
        );
    }

    // println!("PID: {}", next_thread_id);
    // println!("Page table: {:x}", next_thread.page_table as usize);

    restore_context(trap_frame_ptr);

    // println!("Context switched: {}", next_thread_id);
    // println!("thread size: {}", threads.len());
    // println!(
    //     "{:X?}",
    //     unsafe {trap_frame::get(trap_frame_ptr, trap_frame::Register::PC)}
    // );
    // println!(
    //     "{:X?}",
    //     unsafe {trap_frame::get(trap_frame_ptr, trap_frame::Register::SP)}
    // );
}

pub fn fork() -> Option<usize> {
    let threads = unsafe { THREADS.as_mut().unwrap() };
    let current_thread = threads.iter().find(|thread| {
        let state = &thread.state;
        matches!(state, ThreadState::Running(_))
    });

    if current_thread.is_some() {
        let current_thread = current_thread.unwrap();
        let current_thread_context = match &current_thread.state {
            ThreadState::Running(context) => context,
            _ => {
                println_now("FORK: Current thread is not running");
                return None;
            }
        };

        let new_pid = threads.len();
        let program_ptr = unsafe {
            alloc::alloc::alloc(
                Layout::from_size_align(current_thread.program_size, THREAD_ALIGNMENT)
                    .expect("Layout error"),
            )
        };
        let stack_ptr = unsafe {
            alloc::alloc::alloc(
                Layout::from_size_align(current_thread.stack_size, THREAD_ALIGNMENT)
                    .expect("Layout error"),
            )
        };

        let page_table = vm_setup(
            program_ptr,
            current_thread.program_size,
            stack_ptr,
            current_thread.stack_size,
        );

        unsafe {
            core::ptr::copy_nonoverlapping(
                current_thread.program,
                program_ptr,
                current_thread.program_size,
            );
            core::ptr::copy_nonoverlapping(
                current_thread.stack,
                stack_ptr,
                current_thread.stack_size,
            );
        }

        println!("Old pc: {:X}", current_thread_context.pc);
        threads.push(Thread {
            id: new_pid,
            program: program_ptr,
            program_size: current_thread.program_size,
            stack: stack_ptr,
            stack_size: current_thread.stack_size,
            state: ThreadState::Waiting(ThreadContext {
                x0: 0,
                ..*current_thread_context
            }),
            page_table,
        });
        println!("New program start: {:X}", program_ptr as usize);
        // println!("new pc: {:X}", pc);
        println!("New stack start: {:X}", stack_ptr as usize);
        // println!("new sp: {:X}", sp);

        Some(new_pid)
    } else {
        println_now("No running thread to fork");
        None
    }
}

pub fn exec(program_name: String, program_args: Vec<String>) {
    let threads = unsafe { THREADS.as_mut().unwrap() };
    let current_thread = threads.iter_mut().find(|thread| {
        let state = &thread.state;
        matches!(state, ThreadState::Running(_))
    });

    if current_thread.is_none() {
        panic!("No running thread to exec");
    }

    let current_thread = current_thread.unwrap();
    let stack_size = 4096;

    unsafe {
        let filesize = match crate::os::shell::INITRAMFS
            .as_ref()
            .unwrap()
            .get_filesize_by_name(program_name.as_str())
        {
            Some(size) => size as usize,
            None => {
                println_now("File not found");
                return;
            }
        };

        alloc::alloc::dealloc(
            current_thread.program,
            Layout::from_size_align(current_thread.program_size, THREAD_ALIGNMENT)
                .expect("Layout error"),
        );
        alloc::alloc::dealloc(
            current_thread.stack,
            Layout::from_size_align(current_thread.stack_size, THREAD_ALIGNMENT)
                .expect("Layout error"),
        );

        current_thread.program = alloc::alloc::alloc(
            Layout::from_size_align(filesize, THREAD_ALIGNMENT).expect("Layout error"),
        );
        current_thread.stack = alloc::alloc::alloc(
            Layout::from_size_align(4096, THREAD_ALIGNMENT).expect("Layout error"),
        );

        // Clear memory
        core::ptr::write_bytes(current_thread.program, 0, current_thread.program_size);

        INITRAMFS
            .as_ref()
            .unwrap()
            .load_file_to_memory(program_name.as_str(), current_thread.program);

        current_thread.program_size = filesize;
        current_thread.stack_size = stack_size;

        current_thread.page_table = vm_setup(
            current_thread.program,
            current_thread.program_size,
            current_thread.stack,
            current_thread.stack_size,
        );

        asm!(
            "dsb ish",
            "msr ttbr0_el1, {PGD_addr}",
            "tlbi vmalle1is",
            "dsb ish",
            "isb",
            PGD_addr = in(reg) current_thread.page_table
        );
    }

    current_thread.state = ThreadState::Running(ThreadContext {
        pc: 0,
        sp: 0xFFFF_FFFF_FFF0,
        ..ThreadContext::default()
    });
}

pub fn kill(id: usize) {
    let threads = unsafe { THREADS.as_mut().unwrap() };

    let target_thread = threads
        .iter_mut()
        .find(|thread| thread.id == id)
        .expect("Thread not found");

    target_thread.state = ThreadState::Dead;
}

pub fn switch_to_thread(id: Option<usize>, trap_frame_ptr: *mut u64) {
    let threads = unsafe { THREADS.as_mut().unwrap() };

    let target_thread = match id {
        Some(id) => threads
            .iter_mut()
            .find(|thread| thread.id == id)
            .expect("Thread not found"),
        None => threads
            .iter_mut()
            .find(|thread| matches!(&thread.state, ThreadState::Waiting(_)))
            .expect("No running thread"),
    };

    target_thread.state = match &target_thread.state {
        ThreadState::Waiting(context) => ThreadState::Running(context.clone()),
        ThreadState::Running(context) => ThreadState::Running(context.clone()),
        _ => ThreadState::Dead,
    };

    assert!(matches!(target_thread.state, ThreadState::Running(_)));
    
    unsafe {
        asm!(
            "dsb ish",
            "msr ttbr0_el1, {PGD_addr}",
            "tlbi vmalle1is",
            "dsb ish",
            "isb",
            PGD_addr = in(reg) target_thread.page_table
        );
    }
    
    restore_context(trap_frame_ptr);
}
