pub const PERIPHERAL_MMIO_BASE: usize = 0x3f000000;
pub const INTERRUPT_CONTROLLER_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x0000b200;
pub const GPIO_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x00200000;
pub const AUX_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x00215000;
pub const WATCHDOG_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x00100000;
pub const MAILBOX_MMIO_BASE: usize = PERIPHERAL_MMIO_BASE + 0x0000b880;
pub const CORE_TIMER_INTERRUPT_CONTROLL_MMIO_BASE: usize = 0x40000040;
pub const CORE_INTERRUPT_SOURCE_MMIO_BASE: usize = 0x40000060;
