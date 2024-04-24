use crate::timer;
use alloc::boxed::Box;
use alloc::string::String;
use alloc::vec::Vec;
use core::time::Duration;
use stdio::debug;
use stdio::println;

#[inline(never)]
pub fn exec(command: &[u8]) {
    let args = match core::str::from_utf8(&command[10..]) {
        Ok(s) => s.split_whitespace().collect::<Vec<&str>>(),
        Err(_) => {
            debug!("Invalid arguments");
            return;
        }
    };
    if args.len() < 2 {
        debug!("Invalid arguments");
        return;
    }
    // println!("Args: {:?}", args);
    let delay = match args.get(0) {
        Some(s) => match s.parse::<u64>() {
            Ok(n) => n,
            Err(_) => {
                debug!("Invalid delay");
                return;
            }
        },
        None => {
            debug!("Invalid delay");
            return;
        }
    };
    debug!("Delay: {}", delay);
    let message = args[1..].join(" ");
    debug!("Message: {}", message);
    add_timer(Duration::from_millis(delay), message);
}

#[inline(never)]
fn add_timer(duration: Duration, message: String) {
    let tm = timer::manager::get_timer_manager();
    tm.add_timer(
        duration,
        Box::new(move || {
            println!("{}", message.clone());
        }),
    );
}
