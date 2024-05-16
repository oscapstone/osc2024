use crate::sync::mutex::Mutex;
use core::fmt;

static CUR_CONSOLE: Mutex<&'static (dyn Console + Sync)> = Mutex::new(&NULL_CONSOLE);
static NULL_CONSOLE: NullConsole = NullConsole {};

pub enum ConsoleMode {
    Sync,
    Async,
}

pub trait Read {
    /**
     * In Sync mode, it should block the thread and always return Some.
     */
    fn read_char(&self) -> Option<char>;
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

pub trait ReadWrite: Read + Write {}

struct NullConsole {}

impl Read for NullConsole {
    fn read_char(&self) -> Option<char> {
        None
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

impl Console for NullConsole {
    fn change_mode(&self, mode: ConsoleMode) {}
}

pub trait Console: ReadWrite {
    fn change_mode(&self, mode: ConsoleMode);
}

pub fn register_console(new_console: &'static (dyn Console + Sync)) {
    let mut cur_console = CUR_CONSOLE.lock().unwrap();
    *cur_console = new_console;
}

pub fn console() -> &'static (dyn Console + Sync) {
    *CUR_CONSOLE.lock().unwrap()
}
