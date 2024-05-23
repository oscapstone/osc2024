use alloc::{rc::Rc, vec, vec::Vec};
use cpu::cpu::{disable_kernel_space_interrupt, enable_kernel_space_interrupt};
use library::sync::mutex::Mutex;

use crate::memory::{virt_to_phys, AllocatedMemory};

use super::page::{MemoryAccessPermission, MemoryAttribute, PageTable};

#[derive(Debug, Clone, Copy)]
pub enum MemoryProtection {
    None = 0,
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Exec = 4,
    ReadExec = 5,
    WriteExec = 6,
    ReadWriteExec = 7,
}

impl From<MemoryAccessPermission> for MemoryProtection {
    fn from(value: MemoryAccessPermission) -> Self {
        // TODO: check exec permission, now only read and write
        match value {
            MemoryAccessPermission::ReadOnlyEL1 => MemoryProtection::None,
            MemoryAccessPermission::ReadOnlyEL1EL0 => MemoryProtection::ReadExec,
            MemoryAccessPermission::ReadWriteEL1 => MemoryProtection::None,
            MemoryAccessPermission::ReadWriteEL1EL0 => MemoryProtection::ReadWriteExec,
        }
    }
}

impl From<u32> for MemoryProtection {
    fn from(value: u32) -> Self {
        match value {
            0 => MemoryProtection::None,
            1 => MemoryProtection::Read,
            2 => MemoryProtection::Write,
            3 => MemoryProtection::ReadWrite,
            4 => MemoryProtection::Exec,
            5 => MemoryProtection::ReadExec,
            6 => MemoryProtection::WriteExec,
            7 => MemoryProtection::ReadWriteExec,
            _ => panic!("Invalid MemoryProtection"),
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub enum MemoryExecutePermission {
    AllowKernelAndUser,
    AllowUser,
    AllowKernel,
    Deny,
}

#[derive(Debug, Clone)]
pub struct MemoryMappingInfo {
    virt_addr: usize,
    phys_addr: Option<usize>,
    memory_attribute: MemoryAttribute,
    access_permission: MemoryAccessPermission,
    execute_permission: MemoryExecutePermission,
    size: usize,
    physical_memory: Option<Rc<AllocatedMemory>>,
}

impl MemoryMappingInfo {
    pub const fn new(
        virt_addr: usize,
        phys_addr: Option<usize>,
        memory_attribute: MemoryAttribute,
        access_permission: MemoryAccessPermission,
        execute_permission: MemoryExecutePermission,
        size: usize,
        physical_memory: Option<Rc<AllocatedMemory>>,
    ) -> Self {
        Self {
            virt_addr,
            phys_addr,
            memory_attribute,
            access_permission,
            execute_permission,
            size,
            physical_memory,
        }
    }

    pub const fn virt_addr(&self) -> usize {
        self.virt_addr
    }

    pub const fn phys_addr(&self) -> Option<usize> {
        self.phys_addr
    }

    pub const fn memory_attribute(&self) -> MemoryAttribute {
        self.memory_attribute
    }

    pub const fn access_permission(&self) -> MemoryAccessPermission {
        self.access_permission
    }

    pub const fn execute_permission(&self) -> MemoryExecutePermission {
        self.execute_permission
    }

    pub const fn size(&self) -> usize {
        self.size
    }

    pub fn physical_memory(&self) -> Option<Rc<AllocatedMemory>> {
        self.physical_memory.clone()
    }
}

#[derive(Debug, Clone, Copy)]
pub enum DemandPageError {
    RegionNotFound,
    Other(&'static str),
}

#[derive(Debug, Clone, Copy)]
pub enum CopyOnWriteError {
    RegionNotFound,
    RegionIsReadOnly,
    Other(&'static str),
}

#[derive(Debug)]
pub struct MemoryMapping {
    /// list of memory mapping info
    memory_mapping_info_list: Mutex<Vec<MemoryMappingInfo>>,
    /// user space page table
    page_table: Mutex<PageTable>,
}

impl MemoryMapping {
    pub fn new() -> Self {
        Self {
            memory_mapping_info_list: Mutex::new(Vec::new()),
            page_table: Mutex::new(PageTable::new()),
        }
    }

    #[inline(always)]
    pub fn page_table_phys_base_address(&self) -> u64 {
        self.page_table.lock().unwrap().phys_base_address()
    }

    /// Map pages
    /// The function check overlaps with existing memory mapping info.
    /// If there is an overlap, it returns an error.
    /// You can use unmap_pages to remove the memory mapping info and map it again.
    pub fn map_pages(
        &self,
        virt_addr: usize,
        phys_addr: Option<usize>,
        size: usize,
        memory_attribute: MemoryAttribute,
        access_permission: MemoryAccessPermission,
        execute_permission: MemoryExecutePermission,
        physical_memory: Option<Rc<AllocatedMemory>>,
    ) -> Result<(), &'static str> {
        if phys_addr.is_none() && physical_memory.is_some() {
            return Err("phys_addr is required when physical_memory is provided");
        }

        unsafe { disable_kernel_space_interrupt() }
        if self.is_overlaps(virt_addr, size) {
            unsafe { enable_kernel_space_interrupt() }
            return Err("MemoryMappingInfo overlaps");
        }
        let i = self
            .memory_mapping_info_list
            .lock()
            .unwrap()
            .partition_point(|x| x.virt_addr < virt_addr);
        self.memory_mapping_info_list.lock().unwrap().insert(
            i,
            MemoryMappingInfo::new(
                virt_addr,
                phys_addr,
                memory_attribute,
                access_permission,
                execute_permission,
                size,
                physical_memory,
            ),
        );
        unsafe { enable_kernel_space_interrupt() }
        Ok(())
    }

    fn allocate_pages(
        &self,
        virt_addr: usize,
        phys_addr: usize,
        size: usize,
        memory_attribute: MemoryAttribute,
        access_permission: MemoryAccessPermission,
        execute_permission: MemoryExecutePermission,
    ) -> Result<(), &'static str> {
        unsafe { disable_kernel_space_interrupt() }
        let r = self.page_table.lock().unwrap().map_pages(
            virt_addr,
            phys_addr,
            size,
            memory_attribute,
            access_permission,
            execute_permission,
        );
        unsafe { enable_kernel_space_interrupt() }
        r
    }

    pub fn unmap_pages(&self, virt_addr: usize, size: usize) -> Result<(), &'static str> {
        unsafe { disable_kernel_space_interrupt() }
        match self
            .memory_mapping_info_list
            .lock()
            .unwrap()
            .binary_search_by(|x| x.virt_addr.cmp(&virt_addr))
        {
            Ok(index) => self.memory_mapping_info_list.lock().unwrap().remove(index),
            Err(_) => {
                unsafe { enable_kernel_space_interrupt() }
                return Err("MemoryMappingInfo not found");
            }
        };
        let r = self.page_table.lock().unwrap().unmap_pages(virt_addr, size);
        unsafe { enable_kernel_space_interrupt() }
        r
    }

    /// Translate physical address to virtual address by the page table.
    pub fn virt_to_phys(&self, virt_addr: usize) -> Result<usize, &'static str> {
        unsafe {
            PageTable::virt_to_phys_by_table(
                self.page_table.lock().unwrap().phys_base_address() as usize,
                virt_addr,
            )
        }
    }

    pub fn is_overlaps(&self, virt_addr: usize, size: usize) -> bool {
        self.memory_mapping_info_list
            .lock()
            .unwrap()
            .iter()
            .any(|x| {
                let start = x.virt_addr;
                let end = x.virt_addr + x.size;
                let new_start = virt_addr;
                let new_end = virt_addr + size;
                (start <= new_start && new_start < end) // left overlap
                    || (start < new_end && new_end <= end) // right overlap
                    || (new_start <= start && start < new_end) // right overlap
                    || (new_start < end && end <= new_end) // left overlap
            })
    }

    pub fn get_available_virt_addr(&self, size: usize) -> Result<usize, &'static str> {
        if self.memory_mapping_info_list.lock().unwrap().is_empty() {
            return Ok(0);
        }
        let mut start_addr = 0;
        let mut i = 0;

        while start_addr < 0xffff_ffff_ffff {
            let info = &self.memory_mapping_info_list.lock().unwrap()[i];
            if start_addr + size > info.virt_addr {
                start_addr = info.virt_addr + info.size;
                i += 1;
            } else {
                return Ok(start_addr);
            }
        }
        Err("No available virtual address")
    }

    pub fn demand_page(&self, virt_addr: usize) -> Result<(), DemandPageError> {
        unsafe { disable_kernel_space_interrupt() }
        if self.memory_mapping_info_list.lock().unwrap().is_empty() {
            return Err(DemandPageError::RegionNotFound);
        }

        let i = self
            .memory_mapping_info_list
            .lock()
            .unwrap()
            .partition_point(|x| x.virt_addr <= virt_addr);
        if i == 0 {
            return Err(DemandPageError::RegionNotFound);
        }

        let i = i - 1;
        let region = &mut self.memory_mapping_info_list.lock().unwrap()[i];
        if virt_addr >= region.virt_addr + region.size {
            return Err(DemandPageError::RegionNotFound);
        }

        // if region.phys_addr is Some, it means that the region is already allocated.
        let phys_addr = match region.phys_addr {
            Some(phys_addr) => phys_addr,
            None => {
                let allocated_region = Rc::new(AllocatedMemory::new(
                    vec![0_u8; region.size()].into_boxed_slice(),
                ));
                let phys_addr = virt_to_phys(allocated_region.as_ptr() as usize);
                region.phys_addr = Some(phys_addr);
                region.physical_memory = Some(allocated_region);
                phys_addr
            }
        };
        self.allocate_pages(
            region.virt_addr,
            phys_addr,
            region.size,
            region.memory_attribute,
            region.access_permission,
            region.execute_permission,
        )
        .map_err(|e| DemandPageError::Other(e))?;
        unsafe { enable_kernel_space_interrupt() }
        Ok(())
    }

    /// Copy the memory mapping.
    /// The function returns a new MemoryMapping instance with the same memory mapping info.
    /// It use Copy on Write (COW) mechanism for the physical memory.
    pub fn copy(&self) -> Result<Self, &'static str> {
        unsafe { disable_kernel_space_interrupt() }
        let new_memory_mapping = Self::new();
        for info in self.memory_mapping_info_list.lock().unwrap().iter() {
            new_memory_mapping
                .memory_mapping_info_list
                .lock()
                .unwrap()
                .push(MemoryMappingInfo::new(
                    info.virt_addr,
                    info.phys_addr,
                    info.memory_attribute,
                    info.access_permission,
                    info.execute_permission,
                    info.size,
                    info.physical_memory.clone(),
                ));

            if info.physical_memory.is_some() {
                // copy on write
                // set page as read only
                // when permission fault occurs, the page is copied and set as read write if the page is read write originally
                self.page_table.lock().unwrap().map_pages(
                    info.virt_addr,
                    info.phys_addr.unwrap(),
                    info.size,
                    info.memory_attribute,
                    MemoryAccessPermission::ReadOnlyEL1EL0,
                    info.execute_permission,
                )?;
                new_memory_mapping.page_table.lock().unwrap().map_pages(
                    info.virt_addr,
                    info.phys_addr.unwrap(),
                    info.size,
                    info.memory_attribute,
                    MemoryAccessPermission::ReadOnlyEL1EL0,
                    info.execute_permission,
                )?;
            }
        }
        unsafe { enable_kernel_space_interrupt() }
        Ok(new_memory_mapping)
    }

    pub fn get_region(&self, virt_addr: usize) -> Option<MemoryMappingInfo> {
        match self
            .memory_mapping_info_list
            .lock()
            .unwrap()
            .binary_search_by(|x| x.virt_addr.cmp(&virt_addr))
        {
            Ok(i) => Some(self.memory_mapping_info_list.lock().unwrap()[i].clone()),
            Err(_) => None,
        }
    }

    pub fn copy_on_write(&self, virt_addr: usize) -> Result<(), CopyOnWriteError> {
        unsafe { disable_kernel_space_interrupt() }
        if self.memory_mapping_info_list.lock().unwrap().is_empty() {
            return Err(CopyOnWriteError::RegionNotFound);
        }

        let i = self
            .memory_mapping_info_list
            .lock()
            .unwrap()
            .partition_point(|x| x.virt_addr <= virt_addr);
        if i == 0 {
            return Err(CopyOnWriteError::RegionNotFound);
        }

        let i = i - 1;
        let region = &mut self.memory_mapping_info_list.lock().unwrap()[i];
        if virt_addr >= region.virt_addr + region.size {
            return Err(CopyOnWriteError::RegionNotFound);
        }

        let r = match region.access_permission() {
            MemoryAccessPermission::ReadWriteEL1EL0 => {
                let phys_mem = region.physical_memory().unwrap();
                let mut new_mem = vec![0_u8; phys_mem.len()].into_boxed_slice();
                let phys_addr = virt_to_phys(new_mem.as_ptr() as usize);
                new_mem.copy_from_slice(&*phys_mem);
                region.phys_addr = Some(phys_addr);
                region.physical_memory = Some(Rc::new(AllocatedMemory::new(new_mem)));
                self.allocate_pages(
                    region.virt_addr,
                    phys_addr,
                    region.size,
                    region.memory_attribute,
                    MemoryAccessPermission::ReadWriteEL1EL0,
                    region.execute_permission,
                )
                .map_err(|e| CopyOnWriteError::Other(e))
            }
            MemoryAccessPermission::ReadOnlyEL1EL0 => Err(CopyOnWriteError::RegionIsReadOnly),
            _ => unreachable!(),
        };
        unsafe { enable_kernel_space_interrupt() }
        r
    }
}
