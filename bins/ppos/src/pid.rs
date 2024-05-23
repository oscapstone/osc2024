use alloc::rc::{Rc, Weak};
use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};
use hashbrown::HashMap;
use library::sync::mutex::Mutex;

use crate::scheduler::task::Task;

pub type PIDNumber = usize;

#[derive(Debug, Clone, Copy)]
pub enum PIDType {
    PID,
    TGID,
    PGID,
    SID,
    Max,
}

#[derive(Debug, Clone)]
pub struct PID {
    number: PIDNumber,
    tasks: [Option<Weak<Mutex<Task>>>; PIDType::Max as usize],
}

impl PID {
    #[inline(always)]
    pub fn number(&self) -> PIDNumber {
        self.number
    }

    #[inline(always)]
    pub fn pid_task(&self) -> Option<Rc<Mutex<Task>>> {
        self.tasks[PIDType::PID as usize]
            .as_ref()
            .map(|task| Weak::upgrade(&task).unwrap())
    }

    #[inline(always)]
    pub fn set_pid_task(&mut self, task: &Rc<Mutex<Task>>) {
        self.tasks[PIDType::PID as usize] = Some(Rc::downgrade(task));
    }
}

struct PIDManagerInner {
    map: Mutex<HashMap<PIDNumber, Rc<Mutex<PID>>>>,
    current_pid: Mutex<PIDNumber>,
}

impl PIDManagerInner {
    pub fn new() -> Self {
        Self {
            map: Mutex::new(HashMap::new()),
            current_pid: Mutex::new(0),
        }
    }

    fn new_pid(&self) -> Rc<Mutex<PID>> {
        unsafe { disable_kernel_space_interrupt() };
        let mut current_pid = self.current_pid.lock().unwrap();
        let pid = Rc::new(Mutex::new(PID {
            number: *current_pid,
            tasks: [None, None, None, None],
        }));
        self.map.lock().unwrap().insert(*current_pid, pid.clone());
        *current_pid += 1;
        unsafe { enable_kernel_space_interrupt() };
        pid
    }

    fn get_pid(&self, pid: PIDNumber) -> Option<Rc<Mutex<PID>>> {
        self.map.lock().unwrap().get(&pid).map(|pid| pid.clone())
    }

    fn remove_pid(&self, pid: PIDNumber) {
        self.map.lock().unwrap().remove(&pid);
    }
}

pub struct PIDManager {
    inner: Mutex<Option<PIDManagerInner>>,
}

impl PIDManager {
    pub const fn new() -> Self {
        Self {
            inner: Mutex::new(None),
        }
    }

    pub fn init(&self) {
        *self.inner.lock().unwrap() = Some(PIDManagerInner::new());
    }

    pub fn new_pid(&self) -> Rc<Mutex<PID>> {
        self.inner.lock().unwrap().as_ref().unwrap().new_pid()
    }

    pub fn get_pid(&self, pid: PIDNumber) -> Option<Rc<Mutex<PID>>> {
        self.inner.lock().unwrap().as_ref().unwrap().get_pid(pid)
    }

    pub fn remove_pid(&self, pid: PIDNumber) {
        self.inner.lock().unwrap().as_ref().unwrap().remove_pid(pid)
    }
}

pub fn pid_manager() -> &'static PIDManager {
    &PID_MANAGER
}

static PID_MANAGER: PIDManager = PIDManager::new();
