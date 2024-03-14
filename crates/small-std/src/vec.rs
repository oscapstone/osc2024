use core::ops::{self, Range};

pub const VEC_MAX_SIZE: usize = 1024;

#[derive(Debug)]
pub struct Vec<T>
where
    T: Default + Copy,
{
    data: [T; VEC_MAX_SIZE],
    size: usize,
}

impl<T> Vec<T>
where
    T: Default + Copy,
{
    pub fn new() -> Self {
        Self {
            data: [Default::default(); VEC_MAX_SIZE],
            size: 0,
        }
    }

    pub fn push(&mut self, value: T) {
        self.data[self.size] = value;
        self.size += 1;
    }

    pub fn pop(&mut self) -> Option<T> {
        if self.is_empty() {
            return None;
        }
        let value = self.data[self.size];
        self.size -= 1;
        Some(value)
    }

    pub fn clear(&mut self) {
        self.size = 0;
    }

    pub fn len(&self) -> usize {
        self.size
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn iter(&self) -> impl Iterator<Item = &T> {
        self.data.iter()
    }
}

impl<T> ops::Index<usize> for Vec<T>
where
    T: Default + Copy,
{
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        &self.data[index]
    }
}

impl<T> ops::Index<Range<usize>> for Vec<T>
where
    T: Default + Copy,
{
    type Output = [T];

    fn index(&self, index: Range<usize>) -> &Self::Output {
        &self.data[index]
    }
}

impl<T> ops::IndexMut<usize> for Vec<T>
where
    T: Default + Copy,
{
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        &mut self.data[index]
    }
}
