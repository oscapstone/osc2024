// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

//! BSP Memory Management.

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

/// The board's physical memory map.
#[rustfmt::skip]
pub(crate) mod map {

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
        pub const MINI_UART_START:  usize = START + MINI_UART_OFFSET;

    }

    /*
    After rpi3 is booted, some physical memory is already in use. For example, 
    there are already spin tables for multicore boot(0x0000 - 0x1000), f
    latten device tree, initramfs, and your kernel image in the physical memory. 
    Your memory allocator should not allocate these memory blocks if you still need to use them.
    */
    pub mod sdram {

        pub const RAM_START:            usize = 0x1000_0000;
        pub const RAM_END:              usize = 0x8000_0000;
    }

}
