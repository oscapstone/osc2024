pub mod cpu;
pub mod driver;
pub mod memory;

#[allow(dead_code)]
pub fn board_name() -> &'static str {
    "Raspberry Pi 3"
}
