// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

//! BSP Memory Management.

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

/// The board's physical memory map.
#[rustfmt::skip]
pub(super) mod map {

    pub const GPIO_OFFSET:         usize = 0x0020_0000;
    // pub const PL011_UART_OFFSET:         usize = 0x0020_1000;
    pub const MINI_UART_OFFSET:    usize = 0x0021_5000;
    pub const MAILBOX_OFFSET:      usize = 0x0000_B880;

    // There is a VideoCore/ARM MMU translating physical addresses to bus addresses. 
    // The MMU maps physical address 0x3f000000 to bus address 0x7e000000. 
    // In your code, you should use physical addresses instead of bus addresses. 
    // However, the reference uses bus addresses. You should translate them into physical one.
    pub mod mmio {
        use super::*;

        pub const START:            usize =         0x3F00_0000;
        pub const GPIO_START:       usize = START + GPIO_OFFSET;
        pub const MAILBOX_START:    usize = START + MAILBOX_OFFSET;
        // pub const PL011_UART_START: usize = START + UART_OFFSET;
        pub const MINI_UART_START  :usize = START + MINI_UART_OFFSET;

    }

}
