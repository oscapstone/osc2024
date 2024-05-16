use crate::scheduler::current;

pub fn get_pid() -> i32 {
    let current = unsafe { &*current() };
    current.pid_number() as i32
}
