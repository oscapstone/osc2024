#[rustfmt::skip]
pub mod map {

    const GPIO_OFFSET:         usize = 0x0020_0000;
    const MINI_UART_OFFSET:    usize = 0x0021_5000;

    /// Physical devices.
    pub mod mmio {
        use super::*;

        pub const START:            usize =         0x3F00_0000;
        pub const GPIO_START:       usize = START + GPIO_OFFSET;
        pub const MINI_UART_START:  usize = START + MINI_UART_OFFSET;
    }
}
