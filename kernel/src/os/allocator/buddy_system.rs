use super::{print_hex_now, SimpleAllocator, SIMPLE_ALLOCATOR};
use crate::println;
use alloc::boxed::Box;
use core::{
    alloc::{GlobalAlloc, Layout},
    ops::Deref,
    ptr::null_mut,
};

use super::println_now;

static mut FRAME_ROOT: Option<Box<BuddyNode, &SimpleAllocator>> = None;

pub struct BuddyAllocator;

struct BuddyNode<'a> {
    start_address: *mut u8,
    size: usize,
    left_child: Option<Box<BuddyNode<'a>, &'a SimpleAllocator>>,
    right_child: Option<Box<BuddyNode<'a>, &'a SimpleAllocator>>,
    allocated: bool,
}

pub unsafe fn init() {
    FRAME_ROOT = Some(Box::new_in(
        BuddyNode {
            start_address: 0x2000_0000 as *mut u8,
            size: 0x1000_0000,
            left_child: None,
            right_child: None,
            allocated: false,
        },
        &SIMPLE_ALLOCATOR,
    ));
    println_now("Allocated");
}

enum NodeStatus<T> {
    allocated,
    too_small,
    success(T),
}

unsafe fn find_node_recursive(
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

    if child_size >= layout.size() && child_size >= 1024 {
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

        left_child_status = find_node_recursive(current_node.left_child.as_mut().unwrap(), layout);

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

        right_child_status =
            find_node_recursive(current_node.right_child.as_mut().unwrap(), layout);

        if let NodeStatus::success(ret) = right_child_status {
            return right_child_status;
        }
    } else {
        if current_node.left_child.is_some() {
            left_child_status =
                find_node_recursive(current_node.left_child.as_mut().unwrap(), layout);
        }
        if let NodeStatus::success(ret) = left_child_status {
            return left_child_status;
        }
        if current_node.right_child.is_some() {
            right_child_status =
                find_node_recursive(current_node.right_child.as_mut().unwrap(), layout);
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
            //println_now("Found block");
            print_hex_now(current_node.start_address as u32);
            print_hex_now(current_node.size as u32);

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

unsafe impl GlobalAlloc for BuddyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if layout.size() == 0 {
            println!("Warning: Allocating zero size.");
            return null_mut();
        }

        let allocate_status = find_node_recursive(FRAME_ROOT.as_mut().unwrap(), &layout);

        match allocate_status {
            NodeStatus::success(ret) => {
                println_now("Memory allocated");
                ret
            }
            _ => panic!("No memory"),
        }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        match dealloc_recursive(FRAME_ROOT.as_mut().unwrap(), &layout, ptr) {
            true => println_now("Memory freed"),
            false => panic!("Cannot find memory to free"),
        }
    }
}
