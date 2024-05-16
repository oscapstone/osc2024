use crate::scheduler::current;

pub fn exit() {
    unsafe { &mut *current() }.exit(0);
}
