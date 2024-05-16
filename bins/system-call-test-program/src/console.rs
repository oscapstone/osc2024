use core::fmt;

use crate::system_call::{system_call, uart_read};

pub struct Console {}

impl Console {
    pub fn read_char(&self) -> Option<char> {
        let mut buf = [0u8; 1];
        let result = uart_read(buf.as_mut_ptr(), 1);
        if result == 1 {
            Some(buf[0] as char)
        } else {
            None
        }
    }
}

impl fmt::Write for Console {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        unsafe { system_call(2, s.as_ptr() as usize, s.len(), 0, 0, 0, 0) };
        Ok(())
    }
}

static mut CONSOLE: Console = Console {};

pub fn console() -> &'static mut Console {
    unsafe { &mut CONSOLE }
}
