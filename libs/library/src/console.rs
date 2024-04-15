use crate::sync::mutex::Mutex;
use core::fmt;

static CUR_CONSOLE: Mutex<&'static (dyn AsyncReadWrite + Sync)> = Mutex::new(&NULL_CONSOLE);
static NULL_CONSOLE: NullConsole = NullConsole {};

pub trait Read {
    fn read_char(&self) -> char;
}

pub trait Write {
    fn write_char(&self, c: char);

    fn write_str(&self, s: &str) {
        for c in s.chars() {
            self.write_char(c);
        }
    }

    fn write_fmt(&self, args: fmt::Arguments) -> fmt::Result;
}

pub trait AsyncRead {
    fn read_char_async(&self) -> Option<char>;
}

pub trait AsyncWrite {
    fn write_char_async(&self, c: char);

    fn write_str_async(&self, s: &str) {
        for c in s.chars() {
            self.write_char_async(c);
        }
    }
    fn write_fmt_async(&self, args: fmt::Arguments) -> fmt::Result;
}

pub trait ReadWrite: Read + Write {}

pub trait AsyncReadWrite: AsyncRead + AsyncWrite {}

struct NullConsole {}

impl Read for NullConsole {
    fn read_char(&self) -> char {
        ' '
    }
}

impl fmt::Write for NullConsole {
    fn write_str(&mut self, _: &str) -> Result<(), fmt::Error> {
        Ok(())
    }
}

impl Write for NullConsole {
    fn write_char(&self, _: char) {}

    fn write_fmt(&self, _: fmt::Arguments) -> fmt::Result {
        Ok(())
    }
}

impl ReadWrite for NullConsole {}

impl AsyncRead for NullConsole {
    fn read_char_async(&self) -> Option<char> {
        None
    }
}

impl AsyncWrite for NullConsole {
    fn write_char_async(&self, c: char) {}

    fn write_fmt_async(&self, args: fmt::Arguments) -> fmt::Result {
        Ok(())
    }
}

impl AsyncReadWrite for NullConsole {}

pub fn register_console(new_console: &'static (dyn AsyncReadWrite + Sync)) {
    let mut cur_console = CUR_CONSOLE.lock().unwrap();
    *cur_console = new_console;
}

pub fn console() -> &'static (dyn AsyncReadWrite + Sync) {
    *CUR_CONSOLE.lock().unwrap()
}
