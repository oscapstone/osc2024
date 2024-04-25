use super::super::stdio::{print_hex_now, println_now};
use super::{SimpleAllocator, SIMPLE_ALLOCATOR};
use crate::println;
use alloc::boxed::Box;
use core::{
    alloc::{GlobalAlloc, Layout},
    ops::Deref,
    ptr::null_mut,
};

static mut FRAME_ROOT: Option<Box<BuddyNode, &SimpleAllocator>> = None;

pub struct BuddyAllocator;

fn print_node_info(node: &mut BuddyNode) {
    //print_hex_now(node.start_address as u32);
    //print_hex_now(node.size as u32);
}

struct BuddyNode<'a> {
    start_address: *mut u8,
    size: usize,
    left_child: Option<Box<BuddyNode<'a>, &'a SimpleAllocator>>,
    right_child: Option<Box<BuddyNode<'a>, &'a SimpleAllocator>>,
    allocated: bool,
}

pub unsafe fn init() {
    println_now("Before init");
    //print_hex_now((*(0x1000_0000 as *mut u8)) as u32);
    FRAME_ROOT = Some(Box::new_in(
        BuddyNode {
            start_address: 0x0000_0000 as *mut u8,
            size: 0x3C00_0000,
            left_child: None,
            right_child: None,
            allocated: false,
        },
        &SIMPLE_ALLOCATOR,
    ));
    //FRAME_ROOT = None;
    println_now("After init");
}

enum NodeStatus<T> {
    allocated,
    too_small,
    success(T),
}

unsafe fn alloc_recursive(
    current: &mut Box<BuddyNode, &SimpleAllocator>,
    layout: &Layout,
) -> NodeStatus<*mut u8> {
    let current_node = current.as_mut();

    if current_node.allocated {
        return NodeStatus::allocated;
    }

    let mut left_child_status = NodeStatus::too_small;
    let mut right_child_status = NodeStatus::too_small;
    let child_size = current_node.size / 2;

    if child_size >= layout.size() && child_size >= 1024 * 4 {
        // Left child fisrt
        if current_node.left_child.is_none() {
            current_node.left_child = Some(Box::new_in(
                BuddyNode {
                    start_address: current_node.start_address,
                    size: child_size,
                    left_child: None,
                    right_child: None,
                    allocated: false,
                },
                &SIMPLE_ALLOCATOR,
            ));
        }

        left_child_status = alloc_recursive(current_node.left_child.as_mut().unwrap(), layout);

        if let NodeStatus::success(ret) = left_child_status {
            return left_child_status;
        }

        // Right child second
        if current_node.right_child.is_none() {
            current_node.right_child = Some(Box::new_in(
                BuddyNode {
                    start_address: current_node.start_address.add(child_size),
                    size: child_size,
                    left_child: None,
                    right_child: None,
                    allocated: false,
                },
                &SIMPLE_ALLOCATOR,
            ));
        }

        right_child_status = alloc_recursive(current_node.right_child.as_mut().unwrap(), layout);

        if let NodeStatus::success(ret) = right_child_status {
            return right_child_status;
        }
    } else {
        if current_node.left_child.is_some() {
            left_child_status = alloc_recursive(current_node.left_child.as_mut().unwrap(), layout);
        }
        if let NodeStatus::success(ret) = left_child_status {
            return left_child_status;
        }
        if current_node.right_child.is_some() {
            right_child_status =
                alloc_recursive(current_node.right_child.as_mut().unwrap(), layout);
        }
        if let NodeStatus::success(ret) = right_child_status {
            return right_child_status;
        }
    }

    // Check current node itself can cuntain or not.
    if matches!(left_child_status, NodeStatus::too_small)
        && matches!(right_child_status, NodeStatus::too_small)
    {
        let aligned_start = current_node.start_address.align_offset(layout.align());
        let allocated_start = current_node.start_address.add(aligned_start);

        if allocated_start.add(layout.size()) > current_node.start_address.add(current_node.size) {
            return NodeStatus::too_small;
        } else {
            current_node.allocated = true;
            println_now("Allocate block:");
            print_node_info(current_node);

            return NodeStatus::success(allocated_start);
        }
    }

    NodeStatus::allocated
}

unsafe fn dealloc_recursive(
    current: &mut Box<BuddyNode, &SimpleAllocator>,
    layout: &Layout,
    ptr: *mut u8,
) -> bool {
    let current_node = current.as_mut();

    if current_node.allocated {
        if current_node.start_address <= ptr
            && current_node.start_address.add(current_node.size) >= ptr.add(layout.size())
        {
            current_node.allocated = false;
            println_now("Free block:");
            print_node_info(current_node);
            true
        } else {
            false
        }
    } else {
        let mut left_child_status = false;
        let mut right_child_status = false;

        if current_node.left_child.is_some() {
            left_child_status =
                dealloc_recursive(current_node.left_child.as_mut().unwrap(), layout, ptr);
        }

        if current_node.right_child.is_some() {
            right_child_status =
                dealloc_recursive(current_node.right_child.as_mut().unwrap(), layout, ptr);
        }

        left_child_status || right_child_status
    }
}

unsafe fn reserve_recursive(
    current: &mut Box<BuddyNode, &SimpleAllocator>,
    ptr: *mut u8,
    size: usize,
) -> NodeStatus<*mut u8> {
    let current_node = current.as_mut();

    if current_node.start_address <= ptr
        && current_node.start_address.add(current_node.size) >= ptr.add(size)
    {
        let child_size = current_node.size / 2;
        let mut left_child_status = NodeStatus::too_small;
        let mut right_child_status = NodeStatus::too_small;

        // In left child range
        if current_node.start_address <= ptr
            && current_node.start_address.add(child_size) >= ptr.add(size)
        {
            if current_node.left_child.is_none() {
                current_node.left_child = Some(Box::new_in(
                    BuddyNode {
                        start_address: current_node.start_address,
                        size: child_size,
                        left_child: None,
                        right_child: None,
                        allocated: false,
                    },
                    &SIMPLE_ALLOCATOR,
                ));
            }
            left_child_status =
                reserve_recursive(current_node.left_child.as_mut().unwrap(), ptr, size);

            if let NodeStatus::success(ptr) = left_child_status {
                return NodeStatus::success(ptr);
            }

        // In right child range
        } else if current_node.start_address.add(child_size) <= ptr
            && current_node.start_address.add(current_node.size) >= ptr.add(size)
        {
            if current_node.right_child.is_none() {
                current_node.right_child = Some(Box::new_in(
                    BuddyNode {
                        start_address: current_node.start_address.add(child_size),
                        size: child_size,
                        left_child: None,
                        right_child: None,
                        allocated: false,
                    },
                    &SIMPLE_ALLOCATOR,
                ));
            }
            right_child_status =
                reserve_recursive(current_node.right_child.as_mut().unwrap(), ptr, size);

            if let NodeStatus::success(ptr) = right_child_status {
                return NodeStatus::success(ptr);
            }
        }

        if matches!(left_child_status, NodeStatus::too_small)
            && matches!(right_child_status, NodeStatus::too_small)
        {
            current_node.allocated = true;
            return NodeStatus::success(ptr);
        }
    }

    NodeStatus::too_small
}

pub fn reserve(ptr: *mut u8, size: usize) {
    unsafe {
        match reserve_recursive(FRAME_ROOT.as_mut().unwrap(), ptr, size) {
            NodeStatus::success(_) => (),
            _ => {
                panic!("Cannot reserve this memory");
            }
        }
    }
}

unsafe impl GlobalAlloc for BuddyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if layout.size() == 0 {
            println!("Warning: Allocating zero size.");
            return null_mut();
        }

        let allocate_status = alloc_recursive(FRAME_ROOT.as_mut().unwrap(), &layout);

        match allocate_status {
            NodeStatus::success(ret) => {
                //println_now("Memory allocated");
                ret
            }
            _ => panic!("No memory"),
        }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        match dealloc_recursive(FRAME_ROOT.as_mut().unwrap(), &layout, ptr) {
            true => {
                //println_now("Memory freed");
            }
            false => {
                //print_hex_now(ptr as u32);
                //print_hex_now(layout.size() as u32);
                panic!("Cannot find memory to free")
            }
        }
    }
}
