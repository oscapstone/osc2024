// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>

mod null_mbox;

use crate::synchronization::{self, NullLock};

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

/// Console interfaces.
#[allow(dead_code)]
pub mod interface {

    pub trait Boardinfo {
        fn get_board_revision(&self) -> u32 {
            0
        }

        fn get_arm_memory(&self) -> (u32, u32) {
            (0, 0)
        }
    }

    /// Trait alias for a full-fledged console.
    pub trait All: Boardinfo {}
}

//--------------------------------------------------------------------------------------------------
// Global instances
//--------------------------------------------------------------------------------------------------

static CUR_MBOX: NullLock<&'static (dyn interface::All + Sync)> =
    NullLock::new(&null_mbox::NULL_MBOX);

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------
use synchronization::interface::Mutex;

/// Register a new console.
pub fn register_mbox(new_mbox: &'static (dyn interface::All + Sync)) {
    CUR_MBOX.lock(|con| *con = new_mbox);
}

/// Return a reference to the currently registered console.
///
/// This is the global console used by all printing macros.
pub fn mbox() -> &'static dyn interface::All {
    CUR_MBOX.lock(|con| *con)
}

//------------------------------------------------------------------------------
// OS Interface Code
//------------------------------------------------------------------------------
