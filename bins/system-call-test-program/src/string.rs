use core::{fmt, str};

use crate::println;

const STRING_MAX_SIZE: usize = 128;

#[derive(Debug)]
pub struct String {
    pub buffer: [u8; STRING_MAX_SIZE],
    pub current: usize,
}

impl From<&str> for String {
    fn from(v: &str) -> Self {
        let mut s = Self::new();
        s.push_str(v);
        s
    }
}

impl fmt::Display for String {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str(self.as_str())?;
        Ok(())
    }
}

impl fmt::Write for String {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.push_str(s);
        Ok(())
    }

    fn write_char(&mut self, c: char) -> fmt::Result {
        self.push(c);
        Ok(())
    }
}

impl Default for String {
    fn default() -> Self {
        String::new()
    }
}

impl String {
    pub fn new() -> Self {
        Self {
            buffer: [0; STRING_MAX_SIZE],
            current: 0,
        }
    }

    pub fn push(&mut self, c: char) {
        self.buffer[self.current] = c as u8;
        self.current += 1;
    }

    pub fn push_str(&mut self, s: &str) {
        for c in s.chars() {
            self.push(c);
        }
    }

    pub fn pop(&mut self) -> Option<char> {
        if self.is_empty() {
            return None;
        }
        self.current -= 1;
        Some(self.buffer[self.current] as char)
    }

    pub fn clear(&mut self) {
        self.current = 0;
    }

    pub fn len(&self) -> usize {
        self.current
    }

    pub fn is_empty(&self) -> bool {
        self.current == 0
    }

    pub fn trim(&self) -> &str {
        if self.is_empty() {
            return "";
        }
        let mut start = 0;
        let mut end = self.len() - 1;
        while start < self.current && self.buffer[start] as char == ' ' {
            start += 1;
        }
        while end > start && self.buffer[end] as char == ' ' {
            end -= 1;
        }
        str::from_utf8(&self.buffer[start..end + 1]).unwrap()
    }

    pub fn as_str(&self) -> &str {
        str::from_utf8(&self.buffer[0..self.current]).unwrap()
    }
}
