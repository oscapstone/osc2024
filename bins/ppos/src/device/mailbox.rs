use core::{any::Any, cmp::min, slice};

use alloc::{
    rc::{Rc, Weak},
    string::String,
    vec::Vec,
};
use library::{sync::mutex::Mutex, time::Time};
use tock_registers::{
    interfaces::{Readable, Writeable},
    register_bitfields, register_structs,
    registers::{ReadOnly, WriteOnly},
};
use vfs::file::{Umode, UMODE};

use crate::{
    file_system::{
        directory_cache::{DirectoryEntry, DirectoryEntryOperation},
        file::{File, FileOperation},
        inode::{INode, INodeOperation},
        super_block::SuperBlockOperation,
        virtual_file_system,
    },
    memory::phys_to_virt,
};

use super::{
    common::MMIODerefWrapper, device_driver::DeviceDriver, interrupt_controller::InterruptNumber,
};

#[repr(u32)]
enum BufferRequestCode {
    ProcessRequest = 0,
}

#[repr(u32)]
enum BufferResponseCode {
    RequestSuccessful = 0x80000000,
    ErrorParsingRequestBuffer = 0x80000001,
}

#[repr(u32)]
enum TagIdentifier {
    GetBoardRevision = 0x00010002,
    GetARMMemory = 0x00010005,
}

register_bitfields! [
    u32,
    MAILBOX_STATUS [
        FULL OFFSET(31) NUMBITS(1) [
            NOT_FULL = 0,
            FULL = 1,
        ],
        EMPTY OFFSET(30) NUMBITS(1) [
            NOT_EMPTY = 0,
            EMPTY = 1,
        ]
    ],
    MAILBOX_READ [
        DATA OFFSET(4) NUMBITS(28) [],
        CHANNEL OFFSET(0) NUMBITS(4) []
    ],
    MAILBOX_WRITE [
        DATA OFFSET(4) NUMBITS(28) [],
        CHANNEL OFFSET(0) NUMBITS(4) []
    ]
];

register_structs! {
    Registers {
        (0x00 => read: ReadOnly<u32, MAILBOX_READ::Register>),
        (0x04 => _reserved1),
        (0x18 => status: ReadOnly<u32, MAILBOX_STATUS::Register>),
        (0x1c => _reserved2),
        (0x20 => write: WriteOnly<u32, MAILBOX_WRITE::Register>),
        (0x24 => @END),
    }
}

pub struct ARMMemoryInfo {
    pub base_address: u32,
    pub size: u32,
}

struct MailboxInner {
    registers: MMIODerefWrapper<Registers>,
}

impl MailboxInner {
    /**
     * # Safety
     *
     * - The user must ensure to provide a correct MMIO start address.
     */
    const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            registers: MMIODerefWrapper::new(mmio_start_addr),
        }
    }

    fn is_writable(&self) -> bool {
        !self.registers.status.is_set(MAILBOX_STATUS::FULL)
    }

    fn is_readable(&self) -> bool {
        !self.registers.status.is_set(MAILBOX_STATUS::EMPTY)
    }

    fn read(&self, channel: u8) -> *mut u32 {
        loop {
            while !self.is_readable() {
                core::hint::spin_loop();
            }
            let tmp = self.registers.read.get();
            let data = tmp & !(0b1111);
            let data_channel = (tmp & 0b1111) as u8;
            if data_channel == channel {
                // get 28 MSB
                return data as *mut u32;
            }
        }
    }

    fn write(&self, channel: u8, buffer_addr: *mut u32) {
        while !self.is_writable() {
            core::hint::spin_loop();
        }
        // use 28 MSB
        let message_addr = buffer_addr as u32 & !(0b1111);
        self.registers.write.set(message_addr | channel as u32);
    }

    fn call(&self, channel: u8, buffer_addr: *mut u32) -> *mut u32 {
        self.write(channel, buffer_addr);
        self.read(channel)
    }

    fn get_board_revision(&self) -> u32 {
        #[repr(align(16))]
        struct GetBoardRevisionBuffer {
            inner: [u32; 7],
        }
        let mut buffer = GetBoardRevisionBuffer { inner: [0; 7] };
        // set buffer length (in bytes)
        buffer.inner[0] = 7 * 4;
        // set request code
        buffer.inner[1] = BufferRequestCode::ProcessRequest as u32;
        // set tag tag identifier
        buffer.inner[2] = TagIdentifier::GetBoardRevision as u32;
        // set value buffer length (in bytes)
        buffer.inner[3] = 4;
        // set tag request code for request
        buffer.inner[4] = 0;
        // set value buffer
        buffer.inner[5] = 0;
        // set end tag bits
        buffer.inner[6] = 0;
        self.call(8, buffer.inner.as_mut_ptr());
        buffer.inner[5]
    }

    fn get_arm_memory(&self) -> ARMMemoryInfo {
        #[repr(align(16))]
        struct GetARMMemoryBuffer {
            inner: [u32; 8],
        }
        let mut buffer = GetARMMemoryBuffer { inner: [0; 8] };
        // set buffer length (in bytes)
        buffer.inner[0] = 8 * 4;
        // set request code
        buffer.inner[1] = BufferRequestCode::ProcessRequest as u32;
        // set tag tag identifier
        buffer.inner[2] = TagIdentifier::GetARMMemory as u32;
        // set value buffer length (in bytes)
        buffer.inner[3] = 8;
        // set tag request code for request
        buffer.inner[4] = 0;
        // set value buffer
        buffer.inner[5] = 0;
        buffer.inner[6] = 0;
        // set end tag bits
        buffer.inner[7] = 0;
        self.call(8, buffer.inner.as_mut_ptr());
        ARMMemoryInfo {
            base_address: buffer.inner[5],
            size: buffer.inner[6],
        }
    }
}

pub struct Mailbox {
    inner: Mutex<MailboxInner>,
    super_block: Mutex<Option<Rc<MailboxSuperBlock>>>,
}

impl Mailbox {
    /**
     * # Safety
     *
     * - The user must ensure to provide a correct MMIO start address.
     */
    pub const unsafe fn new(mmio_start_addr: usize) -> Self {
        Self {
            inner: Mutex::new(MailboxInner::new(mmio_start_addr)),
            super_block: Mutex::new(None),
        }
    }

    pub fn get_board_revision(&self) -> u32 {
        self.inner.lock().unwrap().get_board_revision()
    }

    pub fn get_arm_memory(&self) -> ARMMemoryInfo {
        self.inner.lock().unwrap().get_arm_memory()
    }

    pub fn call(&self, channel: u8, buffer_addr: *mut u32) -> *mut u32 {
        self.inner.lock().unwrap().call(channel, buffer_addr)
    }
}

impl DeviceDriver for Mailbox {
    type InterruptNumberType = InterruptNumber;

    unsafe fn init(&self) -> Result<(), &'static str> {
        *self.super_block.lock().unwrap() = Some(Rc::new(MailboxSuperBlock::new()));
        // init frame buffer
        #[repr(align(16))]
        struct Buffer {
            inner: [u32; 36],
        };
        let mut buffer = Buffer { inner: [0; 36] };
        buffer.inner[0] = 35 * 4;
        buffer.inner[1] = BufferRequestCode::ProcessRequest as u32;

        buffer.inner[2] = 0x48003; // set phy wh
        buffer.inner[3] = 8;
        buffer.inner[4] = 8;
        buffer.inner[5] = 1024; // FrameBufferInfo.width
        buffer.inner[6] = 768; // FrameBufferInfo.height

        buffer.inner[7] = 0x48004; // set virt wh
        buffer.inner[8] = 8;
        buffer.inner[9] = 8;
        buffer.inner[10] = 1024; // FrameBufferInfo.virtual_width
        buffer.inner[11] = 768; // FrameBufferInfo.virtual_height

        buffer.inner[12] = 0x48009; // set virt offset
        buffer.inner[13] = 8;
        buffer.inner[14] = 8;
        buffer.inner[15] = 0; // FrameBufferInfo.x_offset
        buffer.inner[16] = 0; // FrameBufferInfo.y.offset

        buffer.inner[17] = 0x48005; // set depth
        buffer.inner[18] = 4;
        buffer.inner[19] = 4;
        buffer.inner[20] = 32; // FrameBufferInfo.depth

        buffer.inner[21] = 0x48006; // set pixel order
        buffer.inner[22] = 4;
        buffer.inner[23] = 4;
        buffer.inner[24] = 1; // RGB, not BGR preferably

        buffer.inner[25] = 0x40001; // get framebuffer.inner, gets alignment on request
        buffer.inner[26] = 8;
        buffer.inner[27] = 8;
        buffer.inner[28] = 4096; // FrameBufferInfo.pointer
        buffer.inner[29] = 0; // FrameBufferInfo.size

        buffer.inner[30] = 0x40008; // get pitch
        buffer.inner[31] = 4;
        buffer.inner[32] = 4;
        buffer.inner[33] = 0; // FrameBufferInfo.pitch

        buffer.inner[34] = 0;
        self.call(8, buffer.inner.as_mut_ptr());

        if buffer.inner[20] == 32 && buffer.inner[28] != 0 {
            let addr = phys_to_virt((buffer.inner[28] & 0x3FFFFFFF) as usize);
            let size = buffer.inner[29];
            let dev_folder = virtual_file_system()
                .find_directory_entry(&Some(virtual_file_system().root().unwrap().root), "dev")
                .unwrap();
            let super_block = self.super_block.lock().unwrap().as_ref().unwrap().clone();
            let mini_uart_inode = Rc::new(MailboxINode::new(
                addr as *mut u8,
                size as usize,
                Rc::downgrade(&super_block),
            ));
            super_block.add_inode(mini_uart_inode.clone() as Rc<dyn INodeOperation>);
            let mini_uart_dentry = Rc::new(DirectoryEntry::new(
                Some(Rc::downgrade(&dev_folder)),
                String::from("framebuffer"),
                Rc::downgrade(&(mini_uart_inode as Rc<dyn INodeOperation>)),
                Rc::downgrade(&(super_block.clone() as Rc<dyn SuperBlockOperation>)),
            ));
            virtual_file_system().add_directory_entry(mini_uart_dentry.clone());
            dev_folder.add_child(Rc::downgrade(
                &(mini_uart_dentry as Rc<dyn DirectoryEntryOperation>),
            ));
        } else {
            return Err("Unable to set screen resolution to 1024x768x32");
        }

        Ok(())
    }
}

#[derive(Debug)]
pub struct MailboxSuperBlock {
    inodes: Mutex<Vec<Rc<dyn INodeOperation>>>,
}

impl MailboxSuperBlock {
    pub fn new() -> Self {
        Self {
            inodes: Mutex::new(Vec::with_capacity(1)),
        }
    }
}

impl SuperBlockOperation for MailboxSuperBlock {
    fn add_inode(&self, inode: Rc<dyn INodeOperation>) {
        self.inodes.lock().unwrap().push(inode);
    }

    fn set_root(&self, root: Weak<dyn DirectoryEntryOperation>) {
        unimplemented!()
    }

    fn root(&self) -> Option<Weak<dyn DirectoryEntryOperation>> {
        unimplemented!()
    }
}

#[derive(Debug)]
pub struct MailboxFile {
    inner: File,
}

impl MailboxFile {
    pub fn new(inode: Rc<dyn INodeOperation>) -> Self {
        Self {
            inner: File::new(inode),
        }
    }
}

impl FileOperation for MailboxFile {
    fn write(&self, buf: &[u8], len: usize) -> Result<usize, &'static str> {
        let end = min(self.position() + len, self.inode().size());
        let frame_buffer = unsafe {
            slice::from_raw_parts_mut(
                (Rc::downcast::<MailboxINode>(self.inode() as Rc<dyn Any>))
                    .unwrap()
                    .buffer_addr,
                self.inode().size(),
            )
        };
        for i in self.position()..end {
            frame_buffer[i] = buf[i - self.position()];
        }
        self.set_position(end);
        Ok(end - self.position())
    }

    fn read(&self, buf: &mut [u8], len: usize) -> Result<usize, &'static str> {
        let end = min(self.position() + len, self.inode().size());
        let frame_buffer = unsafe {
            slice::from_raw_parts(
                (Rc::downcast::<MailboxINode>(self.inode() as Rc<dyn Any>))
                    .unwrap()
                    .buffer_addr,
                self.inode().size(),
            )
        };
        for i in self.position()..end {
            buf[i - self.position()] = frame_buffer[i];
        }
        self.set_position(end);
        Ok(end - self.position())
    }

    fn close(&self) {
        self.inner.close()
    }

    fn position(&self) -> usize {
        self.inner.position()
    }

    fn set_position(&self, position: usize) {
        self.inner.set_position(position)
    }

    fn inode(&self) -> Rc<dyn INodeOperation> {
        self.inner.inode()
    }
}

#[derive(Debug)]
pub struct MailboxINode {
    inner: INode,
    buffer_addr: *mut u8,
}

impl MailboxINode {
    pub fn new(buffer_addr: *mut u8, size: usize, super_block: Weak<MailboxSuperBlock>) -> Self {
        Self {
            inner: INode::new(
                Umode::new(UMODE::OWNER_READ::SET),
                0,
                0,
                Time::new(0, 0),
                Time::new(0, 0),
                Time::new(0, 0),
                size,
                super_block,
            ),
            buffer_addr,
        }
    }
}

impl INodeOperation for MailboxINode {
    fn lookup(
        &self,
        parent_directory: Rc<dyn DirectoryEntryOperation>,
        target_name: &str,
    ) -> Option<Rc<dyn DirectoryEntryOperation>> {
        self.inner.lookup(parent_directory, target_name)
    }

    fn create(
        &self,
        umode: Umode,
        name: alloc::string::String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        unimplemented!()
    }

    fn mkdir(
        &self,
        umode: Umode,
        name: alloc::string::String,
        parent: Option<Weak<dyn DirectoryEntryOperation>>,
    ) -> Rc<dyn DirectoryEntryOperation> {
        unimplemented!()
    }

    fn open(&self, inode: Rc<dyn INodeOperation>) -> Rc<dyn FileOperation> {
        Rc::new(MailboxFile::new(inode))
    }

    fn size(&self) -> usize {
        self.inner.size()
    }

    fn super_block(&self) -> Weak<dyn SuperBlockOperation> {
        self.inner.super_block()
    }
}
