use alloc::boxed::Box;

pub trait Callback {
    fn call(&self);
}

impl<F> Callback for F
where
    F: Fn(),
{
    fn call(&self) {
        (self)();
    }
}

pub struct Timer {
    pub expiry: u64,
    callback: Box<dyn Callback>,
}

impl Timer {
    pub fn new<F>(expriy: u64, callback: F) -> Timer
    where
        F: Fn() + 'static,
    {
        Timer {
            expiry: expriy,
            callback: Box::new(callback),
        }
    }

    pub fn trigger(&self) {
        self.callback.call();
    }
}

impl PartialEq for Timer {
    fn eq(&self, other: &Self) -> bool {
        self.expiry == other.expiry
    }
}

impl Eq for Timer {}

impl PartialOrd for Timer {
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other).reverse())
    }
}

impl Ord for Timer {
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        self.expiry.cmp(&other.expiry)
    }
}
