use crate::{
    driver::mailbox,
    memory::{paging::page::PageTable, phys_to_virt},
    scheduler::current,
};

pub fn mbox_call(channel: u8, mbox: *mut u32) -> i32 {
    match unsafe {
        PageTable::virt_to_phys_by_table(
            (&*current())
                .memory_mapping()
                .page_table_phys_base_address() as usize,
            mbox as usize,
        )
    } {
        Ok(phys_addr) => {
            mailbox().call(channel, phys_to_virt(phys_addr) as *mut u32);
            1
        }
        Err(e) => 0,
    }
}
