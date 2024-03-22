enum CpioHeaderField {
    Magic,
    Ino,
    Mode,
    Uid,
    Gid,
    Nlink,
    Mtime,
    Filesize,
    Devmajor,
    Devminor,
    Rdevmajor,
    Rdevminor,
    Namesize,
    Check,
}

struct CpioHeader {
    header_ptr: *const CpioHeader,
}

impl CpioHeader {
    pub fn load(address: *const u8) -> CpioHeader {
        CpioHeader {
            header_ptr: address as *const CpioHeader,
        }
    }

    pub fn get(&self, field: CpioHeaderField) -> u64 {
        let offset = match field {
            CpioHeaderField::Magic => 0,
            CpioHeaderField::Ino => 6,
            CpioHeaderField::Mode => 14,
            CpioHeaderField::Uid => 22,
            CpioHeaderField::Gid => 30,
            CpioHeaderField::Nlink => 38,
            CpioHeaderField::Mtime => 46,
            CpioHeaderField::Filesize => 54,
            CpioHeaderField::Devmajor => 62,
            CpioHeaderField::Devminor => 70,
            CpioHeaderField::Rdevmajor => 78,
            CpioHeaderField::Rdevminor => 86,
            CpioHeaderField::Namesize => 94,
            CpioHeaderField::Check => 102,
        };

        let len = match field {
            CpioHeaderField::Magic => 6,
            _ => 8,
        };

        unsafe {
            let ptr = self.header_ptr as *const u8;
            let mut value: u64 = 0;
            for i in 0..len {
                value |= (ptr.offset(offset as isize + i as isize) as u64) << (i * 8);
            }
            value
        }
    }

    
}