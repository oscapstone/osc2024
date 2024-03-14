pub struct NullConsole;

pub static NULL_CONSOLE: NullConsole = NullConsole;

impl super::Write for NullConsole {
    fn write_char(&self, _c: char) {}

    fn write_fmt(&self, _args: core::fmt::Arguments) -> core::fmt::Result {
        core::fmt::Result::Ok(())
    }
}

impl super::Read for NullConsole {}
impl super::All for NullConsole {}
