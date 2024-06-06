use alloc::{vec, vec::Vec};

#[derive(Debug, Clone)]
pub struct FixedSizeTable<T: Clone> {
    table: Vec<Option<T>>,
    cursor: usize,
    amount: usize,
}

impl<T: Clone> FixedSizeTable<T> {
    pub fn new(size: usize) -> Self {
        Self {
            table: vec![None; size],
            cursor: 0,
            amount: 0,
        }
    }

    pub fn is_empty(&self) -> bool {
        self.amount == 0
    }

    pub fn is_full(&self) -> bool {
        self.amount == self.table.len()
    }

    pub fn size(&self) -> usize {
        self.table.len()
    }

    pub fn add(&mut self, item: T) -> Result<usize, &'static str> {
        if self.is_full() {
            return Err("FixedSizeTable is full");
        }

        let mut index = self.cursor;
        loop {
            if self.table[index].is_some() {
                index = (index + 1) % self.size();
                continue;
            }

            self.table[index] = Some(item);
            self.cursor = index + 1;
            break;
        }
        Ok(index)
    }

    pub fn remove(&mut self, index: usize) -> Result<T, &'static str> {
        if index >= self.size() {
            return Err("index out of range");
        }

        match self.table[index].take() {
            Some(item) => {
                self.amount -= 1;
                Ok(item)
            }
            None => Err("no item at the index"),
        }
    }

    pub fn get(&self, index: usize) -> Result<&T, &'static str> {
        if index >= self.size() {
            return Err("index out of range");
        }

        match &self.table[index] {
            Some(item) => Ok(item),
            None => Err("no item at the index"),
        }
    }
}
