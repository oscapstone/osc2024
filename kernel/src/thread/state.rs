#[derive(Clone, Copy)]
pub enum State {
    Init,
    Ready,
    Running,
    Waiting,
    Zombie,
}
