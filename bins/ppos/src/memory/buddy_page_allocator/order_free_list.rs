use core::alloc::{GlobalAlloc, Layout};

use super::simple_allocator::simple_allocator;

type FrameIndex = usize;

#[derive(Debug)]
pub struct OrderFreeList<'a> {
    head: Option<*mut OrderFreeListNode<'a>>,
}

impl<'a> OrderFreeList<'a> {
    pub const fn new() -> Self {
        Self { head: None }
    }

    pub unsafe fn push_front(&mut self, frame_index: FrameIndex) {
        let node =
            simple_allocator().alloc(Layout::new::<OrderFreeListNode>()) as *mut OrderFreeListNode;
        (*node).frame_index = frame_index;
        (*node).next = self.head;
        (*node).prev = None;
        if let Some(head) = self.head {
            (*head).prev = Some(node);
        }
        self.head = Some(node);
    }

    pub unsafe fn pop_front(&mut self) -> Option<FrameIndex> {
        match self.head {
            Some(head) => {
                let frame_index = (*head).frame_index;
                self.head = (*head).next;
                if self.head.is_some() {
                    (*self.head.unwrap()).prev = None;
                }
                simple_allocator().dealloc(head as *mut u8, Layout::new::<OrderFreeListNode>());
                Some(frame_index)
            }
            None => None,
        }
    }

    #[inline(always)]
    pub fn is_empty(&self) -> bool {
        self.head.is_none()
    }

    pub unsafe fn remove(&mut self, node: *mut OrderFreeListNode<'a>) {
        match (*node).prev {
            Some(prev) => {
                (*prev).next = (*node).next;
            }
            None => {
                self.head = (*node).next;
            }
        }
        if (*node).next.is_some() {
            (*(*node).next.unwrap()).prev = (*node).prev;
        }
        simple_allocator().dealloc(node as *mut u8, Layout::new::<OrderFreeListNode>());
    }

    #[inline(always)]
    pub fn head(&self) -> Option<*const OrderFreeListNode<'a>> {
        match self.head {
            Some(head) => Some(head as *const OrderFreeListNode),
            None => None,
        }
    }
}

#[derive(Debug, Clone)]
pub struct OrderFreeListNode<'a> {
    frame_index: FrameIndex,
    next: Option<*mut OrderFreeListNode<'a>>,
    prev: Option<*mut OrderFreeListNode<'a>>,
}

impl<'a> OrderFreeListNode<'a> {
    #[inline(always)]
    pub fn frame_index(&self) -> FrameIndex {
        self.frame_index
    }

    #[inline(always)]
    pub fn next(&self) -> Option<*const OrderFreeListNode<'a>> {
        match self.next {
            Some(next) => Some(next as *const OrderFreeListNode),
            None => None,
        }
    }

    #[inline(always)]
    pub fn prev(&self) -> Option<*const OrderFreeListNode<'a>> {
        match self.prev {
            Some(prev) => Some(prev as *const OrderFreeListNode),
            None => None,
        }
    }
}
