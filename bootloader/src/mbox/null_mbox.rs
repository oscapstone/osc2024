// SPDX-License-Identifier: MIT OR Apache-2.0
//
// Copyright (c) 2022-2023 Andre Richter <andre.o.richter@gmail.com>

//! Null console.

use super::interface;

//--------------------------------------------------------------------------------------------------
// Public Definitions
//--------------------------------------------------------------------------------------------------

pub struct NullMBOX;

//--------------------------------------------------------------------------------------------------
// Global instances
//--------------------------------------------------------------------------------------------------

pub static NULL_MBOX: NullMBOX = NullMBOX {};

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

impl interface::Boardinfo for NullMBOX {
  /// Passthrough of `args` to the `core::fmt::Write` implementation, but guarded by a Mutex to
  /// serialize access.
  fn get_arm_memory(&self) -> (u32, u32) {
      (0, 0)
  }

  fn get_board_revision(&self) -> u32 {
      0
  }
}


impl interface::All for NullMBOX {}