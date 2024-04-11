pub struct NullConsole;

pub static NULL_CONSOLE: NullConsole = NullConsole;

impl super::Write for NullConsole {
    fn write_char(&self, _c: char) {}

    fn write_fmt(&self, _args: core::fmt::Arguments) -> core::fmt::Result {
        core::fmt::Result::Ok(())
    }

    fn flush(&self) {}
}

impl super::Read for NullConsole {
    fn clear_rx(&self) {}
}

impl super::All for NullConsole {}
