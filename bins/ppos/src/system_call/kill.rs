use crate::scheduler::task::Task;

pub fn kill(pid: i32) {
    Task::kill(pid as usize)
}
