#[derive(Debug, Clone)]
pub struct RingBuffer<const SIZE: usize> {
    inner: [u8; SIZE],
    right_index: usize,
    left_index: usize,
}

impl<const SIZE: usize> RingBuffer<SIZE> {
    pub const fn new() -> Self {
        Self {
            inner: [0; SIZE],
            right_index: 0,
            left_index: 0,
        }
    }

    pub fn push(&mut self, byte: u8) {
        self.inner[self.right_index] = byte;
        self.right_index = (self.right_index + 1) % SIZE;
        if self.right_index == self.left_index {
            self.left_index = (self.left_index + 1) % SIZE;
        }
    }

    pub fn pop(&mut self) -> Option<u8> {
        if self.left_index == self.right_index {
            None
        } else {
            let byte = self.inner[self.left_index];
            self.left_index = (self.left_index + 1) % SIZE;
            Some(byte)
        }
    }

    pub fn is_empty(&self) -> bool {
        self.left_index == self.right_index
    }

    pub fn is_full(&self) -> bool {
        (self.right_index + 1) % SIZE == self.left_index
    }
}
