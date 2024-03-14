use core::cell::UnsafeCell;

/// Any object implementing this trait guarantees exclusive access to the data wrapped within
/// the Mutex for the duration of the provided clousure.
pub trait Mutex {
    /// The type of the data that is wrapped by this mutex.
    type Data;

    /// Locks the mutex and grants the closure temporary mutable access to the wrapped data.
    fn lock<'a, R>(&'a self, f: impl FnOnce(&'a mut Self::Data) -> R) -> R;
}

/// A lock that is currently doing no protection agains concurrent access from other cores.
pub struct NullLock<T>
where
    T: ?Sized,
{
    data: UnsafeCell<T>,
}

unsafe impl<T> Send for NullLock<T> where T: ?Sized + Send {}
unsafe impl<T> Sync for NullLock<T> where T: ?Sized + Send {}

impl<T> NullLock<T> {
    pub const fn new(data: T) -> Self {
        Self {
            data: UnsafeCell::new(data),
        }
    }
}

impl<T> Mutex for NullLock<T> {
    type Data = T;

    fn lock<'a, R>(&'a self, f: impl FnOnce(&'a mut Self::Data) -> R) -> R {
        let data = unsafe { &mut *self.data.get() };

        f(data)
    }
}
