pub mod bump_allocator;

#[global_allocator]
pub static ALLOCATOR: bump_allocator::BumpAllocator = bump_allocator::BumpAllocator::new();