use library::sync::mutex::Mutex;

use crate::interrupt_controller::InterruptNumber;

const MAX_DRIVER_NUM: usize = 6;

pub trait DeviceDriver {
    type InterruptNumberType: core::fmt::Display;

    /**
     * # Safety
     */
    unsafe fn init(&self) -> Result<(), &'static str> {
        Ok(())
    }

    fn register_and_enable_interrupt_handler(
        &'static self,
        interrupt_number: &Self::InterruptNumberType,
    ) -> Result<(), &'static str> {
        panic!(
            "Attempt to enable IRQ {} for device, but driver does not support this",
            interrupt_number
        )
    }
}

#[derive(Copy, Clone)]
pub struct DeviceDriverDescriptor<T: 'static> {
    device_driver: &'static (dyn DeviceDriver<InterruptNumberType = T> + Send + Sync),
    post_init: Option<unsafe fn() -> Result<(), &'static str>>,
    interrupt_number: Option<T>,
}

impl<T> DeviceDriverDescriptor<T> {
    pub const fn new(
        device_driver: &'static (dyn DeviceDriver<InterruptNumberType = T> + Send + Sync),
        post_init: Option<unsafe fn() -> Result<(), &'static str>>,
        interrupt_number: Option<T>,
    ) -> Self {
        Self {
            device_driver,
            post_init,
            interrupt_number,
        }
    }
}

struct DriverManagerInner<T: 'static> {
    next_index: usize,
    descriptors: [Option<DeviceDriverDescriptor<T>>; MAX_DRIVER_NUM],
}

impl<T: 'static + Copy> DriverManagerInner<T> {
    const fn new() -> Self {
        Self {
            next_index: 0,
            descriptors: [None; MAX_DRIVER_NUM],
        }
    }
}

pub struct DriverManager<T: 'static> {
    inner: Mutex<DriverManagerInner<T>>,
}

impl<T: core::fmt::Display + Copy> DriverManager<T> {
    const fn new() -> Self {
        Self {
            inner: Mutex::new(DriverManagerInner::new()),
        }
    }

    pub fn register_driver(&self, driver_descriptor: DeviceDriverDescriptor<T>) {
        let mut inner = self.inner.lock().unwrap();
        let next_index = inner.next_index;
        inner.descriptors[next_index] = Some(driver_descriptor);
        inner.next_index += 1;
    }

    /**
     * # Safety
     *
     * - During init, drivers might do stuff with system-wide impact.
     */
    pub unsafe fn init_drivers_and_interrupts(&self) {
        let inner = self.inner.lock().unwrap();
        inner
            .descriptors
            .iter()
            .filter_map(|x| x.as_ref())
            .for_each(|descriptor| {
                if let Err(e) = descriptor.device_driver.init() {
                    panic!("Error initializing drivers: {}", e);
                }

                if let Some(callback) = &descriptor.post_init {
                    if let Err(e) = callback() {
                        panic!("Error during driver post-init callback: {}", e);
                    }
                }
            });
        inner
            .descriptors
            .iter()
            .filter_map(|x| x.as_ref())
            .for_each(|descriptor| {
                if let Some(interrupt_number) = &descriptor.interrupt_number {
                    if let Err(e) = descriptor
                        .device_driver
                        .register_and_enable_interrupt_handler(interrupt_number)
                    {
                        panic!("Error during driver interrupt handler registration: {}", e);
                    }
                }
            });
    }
}

static DRIVER_MANAGER: DriverManager<InterruptNumber> = DriverManager::new();

pub fn driver_manager() -> &'static DriverManager<InterruptNumber> {
    &DRIVER_MANAGER
}
