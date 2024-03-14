use core::{
    cell::UnsafeCell,
    ops::{Deref, DerefMut},
};

pub struct Mutex<T>
where
    T: ?Sized,
{
    data: UnsafeCell<T>,
}

unsafe impl<T> Send for Mutex<T> where T: ?Sized + Send {}
unsafe impl<T> Sync for Mutex<T> where T: ?Sized + Send {}

impl<T> Mutex<T> {
    pub const fn new(data: T) -> Self {
        Self {
            data: UnsafeCell::new(data),
        }
    }

    pub fn lock(&self) -> Result<MutexGuard<'_, T>, &'static str> {
        unsafe { MutexGuard::new(self) }
    }
}

pub struct MutexGuard<'a, T>
where
    T: 'a + ?Sized,
{
    lock: &'a Mutex<T>,
}

impl<'a, T> MutexGuard<'a, T>
where
    T: ?Sized,
{
    unsafe fn new(lock: &'a Mutex<T>) -> Result<Self, &'static str> {
        Ok(Self { lock })
    }
}

impl<T> Deref for MutexGuard<'_, T>
where
    T: ?Sized,
{
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { &*self.lock.data.get() }
    }
}

impl<T> DerefMut for MutexGuard<'_, T>
where
    T: ?Sized,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { &mut *self.lock.data.get() }
    }
}
