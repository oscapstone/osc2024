use core::{marker::PhantomData, ops};

pub struct MMIODerefWrapper<T> {
    mmio_start_addr: usize,
    phantom: PhantomData<T>,
}

impl<T> MMIODerefWrapper<T> {
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            mmio_start_addr,
            phantom: PhantomData,
        }
    }
}

impl<T> ops::Deref for MMIODerefWrapper<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { &*(self.mmio_start_addr as *const T) }
    }
}

#[derive(Copy, Clone, Debug)]
pub struct BoundedUsize<const LOW: usize, const HIGH: usize>(usize);

impl<const LOW: usize, const HIGH: usize> BoundedUsize<{ LOW }, { HIGH }> {
    pub const LOW: usize = LOW;
    pub const HIGH: usize = HIGH;

    pub fn new(n: usize) -> Self {
        BoundedUsize(n.min(Self::HIGH).max(Self::LOW))
    }

    pub fn failable_new(n: usize) -> Result<Self, &'static str> {
        match n {
            n if n < Self::LOW => Err("Value too low"),
            n if n > Self::HIGH => Err("Value too high"),
            n => Ok(BoundedUsize(n)),
        }
    }

    pub fn set(&mut self, n: usize) {
        *self = BoundedUsize(n.min(Self::HIGH).max(Self::LOW))
    }

    pub fn failable_set(&mut self, n: usize) -> Result<(), &'static str> {
        match n {
            n if n < Self::LOW => Err("Value too low"),
            n if n > Self::HIGH => Err("Value too high"),
            n => {
                *self = BoundedUsize(n);
                Ok(())
            }
        }
    }
}

impl<const LOW: usize, const HIGH: usize> core::ops::Deref for BoundedUsize<{ LOW }, { HIGH }> {
    type Target = usize;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl<const LOW: usize, const HIGH: usize> core::fmt::Display for BoundedUsize<{ LOW }, { HIGH }> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", self.0)
    }
}
