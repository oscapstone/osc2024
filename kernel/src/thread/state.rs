#[derive(Clone, Copy, Debug)]
pub enum State {
    Init,
    Ready,
    Running,
    Waiting,
    Zombie,
}
