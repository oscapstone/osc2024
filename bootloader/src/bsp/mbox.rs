use crate::mbox;

//--------------------------------------------------------------------------------------------------
// Public Code
//--------------------------------------------------------------------------------------------------

/// Return a reference to the console.
pub fn mbox() -> &'static dyn mbox::interface::All {
    &super::driver::MBOX
}