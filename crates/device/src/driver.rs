use small_std::{println, sync::Mutex};

const NUM_DRIVERS: usize = 5;

struct DriverManagerInner {
    next_index: usize,
    descriptors: [Option<DeviceDriverDescriptor>; NUM_DRIVERS],
}

/// Device driver functions.
pub trait DeviceDriver {
    /// Return a compatibility string for identifying the driver.
    fn compatible(&self) -> &'static str;

    /// Called by the kernel to bring up the device.
    unsafe fn init(&self) -> Result<(), &'static str> {
        Ok(())
    }
}

pub type DeviceDriverPostInitCallback = unsafe fn() -> Result<(), &'static str>;

/// A descriptor for device drivers
#[derive(Clone, Copy)]
pub struct DeviceDriverDescriptor {
    device_driver: &'static (dyn DeviceDriver + Sync),
    post_init_callback: Option<DeviceDriverPostInitCallback>,
}

pub struct DriverManager {
    inner: Mutex<DriverManagerInner>,
}

static DRIVER_MANAGER: DriverManager = DriverManager::new();

impl DriverManagerInner {
    pub const fn new() -> Self {
        Self {
            next_index: 0,
            descriptors: [None; NUM_DRIVERS],
        }
    }
}

impl DeviceDriverDescriptor {
    pub fn new(
        device_driver: &'static (dyn DeviceDriver + Sync),
        post_init_callback: Option<DeviceDriverPostInitCallback>,
    ) -> Self {
        Self {
            device_driver,
            post_init_callback,
        }
    }
}

pub fn driver_manager() -> &'static DriverManager {
    &DRIVER_MANAGER
}

impl DriverManager {
    pub const fn new() -> Self {
        Self {
            inner: Mutex::new(DriverManagerInner::new()),
        }
    }

    /// Register a device driver with the kernel.
    pub fn register_driver(&self, descriptor: DeviceDriverDescriptor) {
        let mut inner = self.inner.lock().unwrap();

        let next_index = inner.next_index;
        inner.descriptors[next_index] = Some(descriptor);
        inner.next_index += 1;
    }

    /// Fully initialize all drivers.
    ///
    /// # Safety
    ///
    /// - During init, drivers might do stuff with system-wide impact.
    pub unsafe fn init_drivers(&self) {
        let inner = self.inner.lock().unwrap();
        inner
            .descriptors
            .iter()
            .filter_map(|x| x.as_ref())
            .for_each(|descriptor| {
                // 1. Initialize driver
                if let Err(e) = descriptor.device_driver.init() {
                    panic!(
                        "Error initializing driver: {}: {}",
                        descriptor.device_driver.compatible(),
                        e
                    );
                }

                // 2. Call corresponding post init callback
                let Some(callback) = &descriptor.post_init_callback else {
                    return;
                };

                if let Err(e) = callback() {
                    panic!(
                        "Error during driver post-init callback: {}: {}",
                        descriptor.device_driver.compatible(),
                        e
                    );
                }
            })
    }

    /// Enumerate all registered device drivers.
    pub fn enumerate(&self) {
        let inner = self.inner.lock().unwrap();
        inner
            .descriptors
            .iter()
            .filter_map(|x| x.as_ref())
            .enumerate()
            .for_each(|(idx, descriptor)| {
                println!("    {}. {}", idx + 1, descriptor.device_driver.compatible());
            });
    }
}
