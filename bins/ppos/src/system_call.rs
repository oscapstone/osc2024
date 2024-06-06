use crate::exception::exception_handler::ExceptionContext;

mod chdir;
mod close;
mod exec;
mod exit;
mod fork;
mod get_pid;
mod ioctl;
mod kill;
mod kill_with_signal;
mod lseek64;
mod mbox_call;
mod mkdir;
mod mmap;
mod mount;
mod open;
mod read;
mod sig_return;
mod signal;
mod uart_read;
mod uart_write;
mod write;

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
    Open,
    Close,
    Write,
    Read,
    MakeDirectory,
    Mount,
    ChangeDirectory,
    LSeek64,
    Ioctl,

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
            11 => SystemCallNumber::Open,
            12 => SystemCallNumber::Close,
            13 => SystemCallNumber::Write,
            14 => SystemCallNumber::Read,
            15 => SystemCallNumber::MakeDirectory,
            16 => SystemCallNumber::Mount,
            17 => SystemCallNumber::ChangeDirectory,
            18 => SystemCallNumber::LSeek64,
            19 => SystemCallNumber::Ioctl,
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
            exec::exec(arg0 as *const i8, arg1 as *const *const char) as usize
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
        SystemCallNumber::Open => open::open(arg0 as *const i8, arg1 as u32) as usize,
        SystemCallNumber::Close => close::close(arg0 as i32) as usize,
        SystemCallNumber::Write => write::write(arg0 as i32, arg1 as *const u8, arg2) as usize,
        SystemCallNumber::Read => read::read(arg0 as i32, arg1 as *mut u8, arg2) as usize,
        SystemCallNumber::MakeDirectory => mkdir::mkdir(arg0 as *const i8, arg1 as u32) as usize,
        SystemCallNumber::Mount => mount::mount(
            arg0 as *const i8,
            arg1 as *const i8,
            arg2 as *const i8,
            arg3 as u64,
            arg4 as *const (),
        ) as usize,
        SystemCallNumber::ChangeDirectory => chdir::chdir(arg0 as *const i8) as usize,
        SystemCallNumber::LSeek64 => {
            lseek64::lseek64(arg0 as i32, arg1 as i64, (arg2 as i32).into()) as usize
        }
        SystemCallNumber::Ioctl => {
            ioctl::ioctl(arg0 as i32, arg1 as u64, arg2 as *const u8) as usize
        }
        SystemCallNumber::SignalReturn => {
            sig_return::sig_return();
            0
        }
    };
    context.set_return_value(result as u64);
}
