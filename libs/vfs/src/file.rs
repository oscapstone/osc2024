use library::time::Time;
use tock_registers::{fields::FieldValue, interfaces::Readable, register_bitfields, registers::*};

pub struct Umode(InMemoryRegister<u16, UMODE::Register>);

impl Umode {
    pub fn new(val: FieldValue<u16, UMODE::Register>) -> Self {
        Self(InMemoryRegister::new(val.value))
    }
}

impl From<u16> for Umode {
    fn from(val: u16) -> Self {
        Self(InMemoryRegister::new(val))
    }
}

impl Into<u16> for Umode {
    fn into(self) -> u16 {
        self.0.get()
    }
}

impl core::fmt::Debug for Umode {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "{}", self.0.get())
    }
}

register_bitfields! [
    u16,
    pub UMODE [
        OWNER_READ OFFSET(8) NUMBITS(1) [],
        OWNER_WRITE OFFSET(7) NUMBITS(1) [],
        OWNER_EXECUTE OFFSET(6) NUMBITS(1) [],
        GROUP_READ OFFSET(5) NUMBITS(1) [],
        GROUP_WRITE OFFSET(4) NUMBITS(1) [],
        GROUP_EXECUTE OFFSET(3) NUMBITS(1) [],
        OTHER_READ OFFSET(2) NUMBITS(1) [],
        OTHER_WRITE OFFSET(1) NUMBITS(1) [],
        OTHER_EXECUTE OFFSET(0) NUMBITS(1) [],
    ]
];

#[derive(Debug, Clone)]
pub struct FileMetadata {
    pub umode: u16,
    pub uid: u32,
    pub gid: u32,
    /**
     * File last access time
     */
    pub atime: Time,
    /**
     * File content change time
     */
    pub mtime: Time,
    /**
     * File struct change time
     */
    pub ctime: Time,
}
