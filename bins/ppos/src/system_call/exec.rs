use cpio::CPIOArchive;

use crate::{
    memory::{self, phys_to_virt},
    scheduler::current,
};

pub fn exec(name: *const i8, argv: *const *const char) -> i32 {
    let filename = unsafe { core::ffi::CStr::from_ptr(name).to_str().unwrap() };
    let mut devicetree = unsafe {
        devicetree::FlattenedDevicetree::from_memory(phys_to_virt(memory::DEVICETREE_START_ADDR))
    };
    let result = devicetree.traverse(&move |device_name, property_name, property_value| {
        if property_name == "linux,initrd-start" {
            let mut cpio_archive = unsafe {
                CPIOArchive::from_memory(phys_to_virt(u32::from_be_bytes(
                    property_value.try_into().unwrap(),
                ) as usize))
            };

            while let Some(file) = cpio_archive.read_next() {
                if file.name != filename {
                    continue;
                }

                unsafe { &mut *current() }.run_user_program(file.content);
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
