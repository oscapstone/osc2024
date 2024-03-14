use core::str::{self, FromStr};

use super::vec::{Vec, VEC_MAX_SIZE};

#[derive(Debug)]
pub struct String(Vec<u8>);

impl String {
    pub fn new() -> Self {
        Self(Vec::new())
    }

    pub fn push(&mut self, c: char) {
        self.0.push(c as u8);
    }

    pub fn push_str(&mut self, s: &str) {
        for c in s.chars() {
            self.push(c);
        }
    }

    pub fn pop(&mut self) -> Option<char> {
        self.0.pop().map(|c| c as char)
    }

    pub fn clear(&mut self) {
        self.0.clear()
    }

    pub fn len(&self) -> usize {
        self.0.len()
    }

    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }

    pub fn trim(&self) -> &str {
        if self.is_empty() {
            return "";
        }
        let mut start = 0;
        let mut end = self.len() - 1;
        while start < self.len() && self.0[start] as char == ' ' {
            start += 1;
        }
        while end > start && self.0[start] as char == ' ' {
            end -= 1;
        }

        str::from_utf8(&self.0[start..end + 1]).unwrap()
    }

    pub fn as_str(&self) -> &str {
        str::from_utf8(&self.0[0..self.len()]).unwrap()
    }
}

impl FromStr for String {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        if s.len() > VEC_MAX_SIZE {
            return Err("string to large");
        }

        let mut result = Self::new();
        result.push_str(s);
        Ok(result)
    }
}
