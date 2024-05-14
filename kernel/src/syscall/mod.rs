use crate::scheduler;
pub fn get_pid() -> u64 {
    let pid = scheduler::get().current.unwrap() as u64;
    pid
}
