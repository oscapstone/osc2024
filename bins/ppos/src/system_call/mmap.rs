use aarch64_cpu::registers::Readable;
use tock_registers::{register_bitfields, registers::*};

use crate::{
    memory::{
        paging::{
            memory_mapping::MemoryExecutePermission,
            page::{MemoryAccessPermission, MemoryAttribute},
        },
        round_up, PAGE_SIZE,
    },
    scheduler::current,
};

register_bitfields! [
    u32,
    PROTECTION [
        PROT_EXEC OFFSET(2) NUMBITS(1) [],
        PROT_WRITE OFFSET(1) NUMBITS(1) [],
        PROT_READ OFFSET(0) NUMBITS(1) [],
        PROT_NONE OFFSET(0) NUMBITS(1) [
            NOT_ACCESSIBLE = 0,
        ],
    ],
    FLAGS [
        MAP_POPULATE OFFSET(15) NUMBITS(1) [],
        MAP_ANONYMOUS OFFSET(5) NUMBITS(1) [],
    ]
];

pub fn mmap(addr: usize, len: usize, prot: u32, flags: u32, fd: u32, file_offset: u32) -> *mut u8 {
    let prot_reg = InMemoryRegister::<u32, PROTECTION::Register>::new(prot);
    let flags_reg = InMemoryRegister::<u32, FLAGS::Register>::new(flags);
    let len = round_up(len);
    let current = unsafe { &*current() };
    let allocated_addr =
        if addr != 0 && addr % PAGE_SIZE != 0 && !current.memory_mapping().is_overlaps(addr, len) {
            addr
        } else {
            match current.memory_mapping().get_available_virt_addr(len) {
                Ok(addr) => addr,
                Err(_) => return core::ptr::null_mut(),
            }
        };
    if flags_reg.is_set(FLAGS::MAP_ANONYMOUS) {
        if current
            .memory_mapping()
            .map_pages(
                allocated_addr,
                None,
                len,
                MemoryAttribute::Normal,
                if prot_reg.is_set(PROTECTION::PROT_WRITE) {
                    MemoryAccessPermission::ReadWriteEL1EL0
                } else {
                    MemoryAccessPermission::ReadOnlyEL1EL0
                },
                if prot_reg.is_set(PROTECTION::PROT_EXEC) {
                    MemoryExecutePermission::AllowUser
                } else {
                    MemoryExecutePermission::Deny
                },
                None,
            )
            .is_err()
        {
            return core::ptr::null_mut();
        }
        return allocated_addr as *mut u8;
    }
    core::ptr::null_mut()
}
