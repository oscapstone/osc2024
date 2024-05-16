use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};

use crate::{pid::pid_manager, signal::Signal};

pub fn kill_with_signal(pid: i32, signal: i32) {
    let pid = pid_manager().get_pid(pid as usize);
    if pid.is_none() {
        return;
    }
    let pid = pid.unwrap();
    unsafe { disable_kernel_space_interrupt() };
    pid.lock()
        .unwrap()
        .pid_task()
        .unwrap()
        .lock()
        .unwrap()
        .send_signal(Signal::from(signal as usize));
    unsafe { enable_kernel_space_interrupt() };
}
