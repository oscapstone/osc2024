use driver::watchdog;

pub fn exec() {
    watchdog::reset(100);
}
