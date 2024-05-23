use crate::exception::exception_handler::ExceptionContext;

mod exec;
mod exit;
mod fork;
mod get_pid;
mod kill;
mod kill_with_signal;
mod mbox_call;
mod mmap;
mod sig_return;
mod signal;
mod uart_read;
mod uart_write;

pub enum SystemCallNumber {
    GetPID,
    UARTRead,
    UARTWrite,
    Exec,
    Fork,
    Exit,
    MBoxCall,
    Kill,
    Signal,
    KillWithSignal,
    MMap,

    // internal use
    SignalReturn = 64,
}

impl From<u64> for SystemCallNumber {
    fn from(value: u64) -> Self {
        match value {
            0 => SystemCallNumber::GetPID,
            1 => SystemCallNumber::UARTRead,
            2 => SystemCallNumber::UARTWrite,
            3 => SystemCallNumber::Exec,
            4 => SystemCallNumber::Fork,
            5 => SystemCallNumber::Exit,
            6 => SystemCallNumber::MBoxCall,
            7 => SystemCallNumber::Kill,
            8 => SystemCallNumber::Signal,
            9 => SystemCallNumber::KillWithSignal,
            10 => SystemCallNumber::MMap,
            64 => SystemCallNumber::SignalReturn,
            _ => panic!("unsupport system call number"),
        }
    }
}

pub fn system_call(
    context: &mut ExceptionContext,
    arg0: usize,
    arg1: usize,
    arg2: usize,
    arg3: usize,
    arg4: usize,
    arg5: usize,
) {
    let result = match context.system_call_number().into() {
        SystemCallNumber::GetPID => get_pid::get_pid() as usize,
        SystemCallNumber::UARTRead => uart_read::uart_read(arg0 as *mut u8, arg1),
        SystemCallNumber::UARTWrite => uart_write::uart_write(arg0 as *const u8, arg1),
        SystemCallNumber::Exec => {
            exec::exec(arg0 as *const char, arg1 as *const *const char) as usize
        }
        SystemCallNumber::Fork => fork::fork() as usize,
        SystemCallNumber::Exit => {
            exit::exit();
            0
        }
        SystemCallNumber::MBoxCall => mbox_call::mbox_call(arg0 as u8, arg1 as *mut u32) as usize,
        SystemCallNumber::Kill => {
            kill::kill(arg0 as i32);
            0
        }
        SystemCallNumber::Signal => {
            let handler = unsafe {
                core::mem::transmute::<*const fn() -> !, fn() -> !>(arg1 as *const fn() -> !)
            };
            signal::signal(arg0 as i32, handler);
            0
        }
        SystemCallNumber::KillWithSignal => {
            kill_with_signal::kill_with_signal(arg0 as i32, arg1 as i32);
            0
        }
        SystemCallNumber::MMap => mmap::mmap(
            arg0,
            arg1,
            arg2 as u32,
            arg3 as u32,
            arg4 as u32,
            arg5 as u32,
        ) as usize,
        SystemCallNumber::SignalReturn => {
            sig_return::sig_return();
            0
        }
    };
    context.set_return_value(result as u64);
}
