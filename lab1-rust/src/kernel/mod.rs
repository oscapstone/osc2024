// mod command;
mod console;
use crate::bsp::gpio;
use crate::bsp::mini_uart;
// use command::hello::hello_command;
use console::Console;

static GPIO: gpio::GPIO = unsafe { gpio::GPIO::new() };
static MINI_UART: mini_uart::MiniUart = unsafe { mini_uart::MiniUart::new() };

// static mut BUFFER: [u8; 1024] = [0; 1024];
// pub fn get_line() -> &'static str {
//     let mut index = 0;
//
//     loop {
//         let c = MINI_UART.receive();
//         MINI_UART.send(c);
//         if c == b'\r' || index >= 1024 {
//             break;
//         }
//         unsafe {
//             BUFFER[index] = c;
//         }
//         index += 1;
//     }
//
//     unsafe { core::str::from_utf8_unchecked(&BUFFER[0..index]) }
// }
// pub fn get_line(buffer: &mut [u8; 5]) -> usize {
//     let mut index = 0;
//     loop {
//         let c = MINI_UART.receive();
//         MINI_UART.send(c);
//         if c == b'\r' || index >= 5 {
//             break;
//         }
//         buffer[index] = c;
//         index += 1;
//     }
//     index
// }
pub fn get_line() {
    // let mut s: [u8; 128] = [10; 128];
    // for i in 0..127 {
    //     s[i] = MINI_UART.receive();
    //     MINI_UART.send(s[i]);
    //     if s[i] == 10 {
    //         return (i, s);
    //     }
    // }
    // MINI_UART.send(b'\n');
    // (127, s)
    MINI_UART.send(MINI_UART.receive());
}

pub fn println(s: &str) {
    for c in s.bytes() {
        MINI_UART.send(c);
    }
    MINI_UART.send(b'\r');
    MINI_UART.send(b'\n');
}

pub fn print(s: &str) {
    for c in s.bytes() {
        MINI_UART.send(c);
    }
}

pub fn init() -> ! {
    MINI_UART.setup_mini_uart();
    GPIO.init();
    let console = Console::new();
    // let _ = console.register_command(hello_command());

    println("Hello, world!");

    loop {
        // println("");
        console.run();
        // println(get_line());
        // let mut tbuffer: [u8; 5] = [1, 2, 3, 4, 5];
        // let len = get_line(&mut tbuffer);

        // mini_uart::send(tbuffer[0]);
        // for _i in 0..5 {
        //     mini_uart::send(b'0');
        // }
        // console.run();
    }
}
