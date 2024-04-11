use super::{
    spec::{FdtProperty, StructureBlockToken},
    DeviceTreeEntryValue,
};

#[derive(Debug, Clone, Copy)]
pub struct DeviceTreeProperty<'a> {
    pub name: &'a str,
    pub value: DeviceTreeEntryValue<'a>,
}

#[derive(Debug, Clone, Copy)]
pub enum DeviceTreeToken {
    BeginNode { name: &'static str },
    EndNode,
    Property(DeviceTreeProperty<'static>),
}

#[derive(Debug, Clone, Copy)]
pub enum ParseTokenError {
    InvalidToken(u32),
    OutOfBounds,
}

impl core::fmt::Display for ParseTokenError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            ParseTokenError::InvalidToken(token) => {
                write!(f, "Invalid token: 0x{:08x}", token)
            }
            ParseTokenError::OutOfBounds => write!(
                f,
                "Out of bounds while parsing tokens, please check the input addresses"
            ),
        }
    }
}

impl core::error::Error for ParseTokenError {}

pub fn parse_tokens(start_address: usize, end_address: usize, strings_address: usize) -> Iter {
    Iter::new(start_address, end_address, strings_address)
}

#[derive(Debug, Clone, Copy)]
pub struct Iter {
    address: usize,
    end_address: usize,
    strings_address: usize,
}

impl Iter {
    fn new(address: usize, end_address: usize, strings_address: usize) -> Self {
        Self {
            address,
            end_address,
            strings_address,
        }
    }
}

impl Iterator for Iter {
    type Item = Result<DeviceTreeToken, ParseTokenError>;

    fn next(&mut self) -> Option<Self::Item> {
        while self.address < self.end_address {
            let token = unsafe { u32::from_be(*(self.address as *const u32)) };
            let Ok(token) = StructureBlockToken::try_from(token) else {
                return Some(Err(ParseTokenError::InvalidToken(token)));
            };
            self.address += 4;
            match token {
                StructureBlockToken::FdtNop => continue,
                StructureBlockToken::FdtEndNode => return Some(Ok(DeviceTreeToken::EndNode)),
                StructureBlockToken::FdtEnd => return None,
                StructureBlockToken::FdtBeginNode => {
                    let name = unsafe { parse_string(self.address) };
                    self.address = align_to(self.address + name.len() + 1, 4);
                    return Some(Ok(DeviceTreeToken::BeginNode { name }));
                }
                StructureBlockToken::FdtProp => {
                    let prop = unsafe { parse_property(self.address, self.strings_address) };
                    self.address = align_to(
                        self.address + core::mem::size_of::<FdtProperty>() + prop.size as usize,
                        4,
                    );
                    return Some(Ok(DeviceTreeToken::Property(DeviceTreeProperty {
                        name: prop.name,
                        value: prop.value,
                    })));
                }
            }
        }

        Some(Err(ParseTokenError::OutOfBounds))
    }
}

#[inline(always)]
fn align_to(addr: usize, align: usize) -> usize {
    let offset = (addr as *const u8).align_offset(align) as usize;
    addr + offset
}

/// Parse a null-terminatd string from the given address.
/// # Safety
/// The caller must ensure that the address is valid and points to a null-terminated string.
unsafe fn parse_string(address: usize) -> &'static str {
    let name_start = address as *const u8;
    let name_len = {
        let mut name_end = name_start;
        while *name_end != 0 {
            name_end = name_end.add(1);
        }
        name_end as usize - name_start as usize
    };
    core::str::from_utf8_unchecked(core::slice::from_raw_parts(name_start, name_len))
}

#[derive(Debug)]
struct DeviceTreeEntry<'a> {
    name: &'a str,
    size: u32,
    value: DeviceTreeEntryValue<'a>,
}

/// Parse a FDT_PROP entry from the given address.
/// # Safety
/// The caller must ensure that the address is valid and points to a valid property.
unsafe fn parse_property(address: usize, strings_address: usize) -> DeviceTreeEntry<'static> {
    let prop = &*(address as *const FdtProperty);
    let name = parse_string(strings_address + prop.nameoff() as usize);
    let size = prop.len();
    let value = core::slice::from_raw_parts(
        (address + core::mem::size_of::<FdtProperty>()) as *const u8,
        size as usize,
    );
    let value = match size {
        4 => DeviceTreeEntryValue::U32(u32::from_be_bytes(value.try_into().unwrap())),
        8 => DeviceTreeEntryValue::U64(u64::from_be_bytes(value.try_into().unwrap())),
        _ => {
            let string_or_stringlist = value
                .iter()
                .all(|&b| b.is_ascii_graphic() || b.is_ascii_whitespace() || b == 0);

            if string_or_stringlist {
                let value = core::str::from_utf8_unchecked(value);
                DeviceTreeEntryValue::String(value)
            } else {
                DeviceTreeEntryValue::Bytes(value)
            }
        }
    };

    DeviceTreeEntry {
        name,
        size: prop.len(),
        value,
    }
}
