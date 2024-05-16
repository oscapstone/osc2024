use crate::scheduler::{current, task};

pub enum Signal {
    SIGHUP = 1,
    SIGINT = 2,
    SIGQUIT = 3,
    SIGILL = 4,
    SIGTRAP = 5,
    SIGABRT = 6,
    SIGFPE = 8,
    SIGKILL = 9,
    SIGSEGV = 11,
    SIGPIPE = 13,
    SIGALRM = 14,
    SIGTERM = 15,
    Max = 16,
}

impl From<usize> for Signal {
    fn from(value: usize) -> Self {
        match value {
            1 => Signal::SIGHUP,
            2 => Signal::SIGINT,
            3 => Signal::SIGQUIT,
            4 => Signal::SIGILL,
            5 => Signal::SIGTRAP,
            6 => Signal::SIGABRT,
            8 => Signal::SIGFPE,
            9 => Signal::SIGKILL,
            11 => Signal::SIGSEGV,
            13 => Signal::SIGPIPE,
            14 => Signal::SIGALRM,
            15 => Signal::SIGTERM,
            _ => panic!("unknown signal"),
        }
    }
}

pub fn default_kill_handler() {
    (unsafe { &mut *current() }).exit(0);
}
