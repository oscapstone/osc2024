#[derive(Clone, Copy, Debug)]
pub enum State {
    Ready,
    Running,
    Waiting,
    Zombie,
}
