#![no_main]
#![no_std]
#![feature(allocator_api)]
#![feature(btreemap_alloc)]
#![feature(ptr_mask)]

mod cpu;
mod os;
extern crate alloc;
