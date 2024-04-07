mod parser;
mod spec;

use self::{
    parser::{DeviceTreeProperty, DeviceTreeToken, ParseTokenError},
    spec::FdtHeader,
};

#[derive(Debug, Clone, Copy)]
pub enum DeviceTreeError {
    InvalidMagic(u32),
    UnsupportedVersion(u32),
    ParseTokenError(ParseTokenError),
}

impl core::fmt::Display for DeviceTreeError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            DeviceTreeError::InvalidMagic(magic) => {
                write!(f, "Invalid magic number: 0x{:x}", magic)
            }
            DeviceTreeError::UnsupportedVersion(version) => {
                write!(f, "Unsupported version: {}", version)
            }
            DeviceTreeError::ParseTokenError(e) => write!(f, "{}", e),
        }
    }
}

impl core::error::Error for DeviceTreeError {}

pub struct DeviceTree {
    base_address: usize,
}

impl DeviceTree {
    /// Create a new DeviceTree instance.
    ///
    /// # Safety
    /// - The caller must ensure that the base address is valid.
    pub unsafe fn new(base_address: usize) -> Self {
        Self { base_address }
    }

    /// Traverse the device tree using the given function.
    ///
    /// # Example
    ///
    /// ```rust
    /// let dt = unsafe { DeviceTree::new(0x1234_5678) };
    /// let value = dt.traverse(|node /* : &str */, props: /* Iter<'a> */| {
    ///     println!("node '{}':", node);
    ///     for prop in props {
    ///         let prop = prop.unwrap();
    ///         match prop.value {
    ///             DeviceTreeEntryValue::U32(v) => println!("  - {}: {:#x}", prop.name, v),
    ///             DeviceTreeEntryValue::U64(v) => println!("  - {}: {:#x}", prop.name, v),
    ///             DeviceTreeEntryValue::String(v) => println!("  - {}: {}", prop.name, v),
    ///             DeviceTreeEntryValue::Bytes(v) => println!("  - {}: {:?}", prop.name, v),
    ///         }
    ///         // Do something with the property
    ///    }
    /// });
    /// ```
    pub fn traverse<F>(&self, mut traverse_fn: F) -> Result<(), DeviceTreeError>
    where
        F: FnMut(&str, Iter),
    {
        let header = unsafe { &*(self.base_address as *const FdtHeader) };
        if !header.is_valid() {
            return Err(DeviceTreeError::InvalidMagic(header.magic()));
        }
        if header.version() != 17 {
            return Err(DeviceTreeError::UnsupportedVersion(header.version()));
        }

        let dt_struct_start = self.base_address + header.off_dt_struct() as usize;
        let dt_struct_end = dt_struct_start + header.size_dt_struct() as usize;
        let dt_strings_start = self.base_address + header.off_dt_strings() as usize;

        let mut tokens = parser::parse_tokens(dt_struct_start, dt_struct_end, dt_strings_start);
        while let Some(token) = tokens.next() {
            let token = token.map_err(|e| DeviceTreeError::ParseTokenError(e))?;
            match token {
                DeviceTreeToken::BeginNode { name } => {
                    let it = Iter::new(tokens.clone());
                    traverse_fn(name, it);
                }
                _ => {}
            }
        }

        Ok(())
    }
}

pub struct Iter {
    inner: parser::Iter,
}

impl Iter {
    fn new(inner: parser::Iter) -> Self {
        Self { inner }
    }
}

impl Iterator for Iter {
    type Item = Result<DeviceTreeProperty<'static>, DeviceTreeError>;

    fn next(&mut self) -> Option<Self::Item> {
        let token = match self.inner.next()? {
            Ok(token) => token,
            Err(e) => return Some(Err(DeviceTreeError::ParseTokenError(e))),
        };

        match token {
            DeviceTreeToken::Property(prop) => Some(Ok(prop)),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub enum DeviceTreeEntryValue<'a> {
    U32(u32),
    U64(u64),
    String(&'a str),
    Bytes(&'a [u8]),
}
