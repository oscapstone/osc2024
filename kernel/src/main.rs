#![no_main]
#![no_std]
#![feature(allocator_api)]
#![feature(btreemap_alloc)]
#![feature(ptr_mask)]
#![allow(unused_variables)]
#![allow(non_camel_case_types)]

mod cpu;
mod os;
extern crate alloc;
