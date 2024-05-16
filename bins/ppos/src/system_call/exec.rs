use alloc::{boxed::Box, vec::Vec};
use cpio::CPIOArchive;

use crate::{
    memory,
    scheduler::{scheduler, task::Task},
};

pub fn exec(name: *const char, argv: *const *const char) -> i32 {
    let filename = unsafe {
        core::ffi::CStr::from_ptr(name as *const i8)
            .to_str()
            .unwrap()
    };
    let mut devicetree =
        unsafe { devicetree::FlattenedDevicetree::from_memory(memory::DEVICETREE_START_ADDR) };
    let result = devicetree.traverse(&move |device_name, property_name, property_value| {
        if property_name == "linux,initrd-start" {
            let mut cpio_archive = unsafe {
                CPIOArchive::from_memory(
                    u32::from_be_bytes(property_value.try_into().unwrap()) as usize
                )
            };

            while let Some(file) = cpio_archive.read_next() {
                if file.name != filename {
                    continue;
                }
                let mut code = Vec::from(file.content).into_boxed_slice();
                let code_ptr = code.as_mut_ptr();
                Box::into_raw(code);
                let mut task =
                    Task::from_job(unsafe { core::mem::transmute::<*mut u8, fn() -> !>(code_ptr) });
                scheduler().execute_task(task);
                return Ok(());
            }
            return Ok(());
        }
        Ok(())
    });
    match result {
        Ok(_) => 0,
        Err(_) => 1,
    }
}
