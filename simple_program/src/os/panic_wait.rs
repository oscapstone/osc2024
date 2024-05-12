use core::panic::PanicInfo;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    // println(info.to_string().as_str());
    loop {}
}
