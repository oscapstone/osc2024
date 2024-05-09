// #[repr(C)]
// #[derive(Clone, Copy)]
// pub struct FdtNodeHeader<'a> {
//     name: &'a [u8],
//     tag: u32,
// }

// #[repr(C)]
// #[derive(Clone, Copy)]
// pub struct FdtProperty<'a> {
//     tag: u32,
//     len: u32,
//     nameoff: u32,
//     data: &'a [u8], // Flexible array member not directly representable in Rust
// }
