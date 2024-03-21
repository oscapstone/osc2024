use crate::cpu::uart::{recv_non_blocking};
use crate::stdio::{print, println, print_dec};

const KERNEL_LOAD_ADDRESS: u32 = 0x80000;

pub fn load_kernel() {
    let mut ptr = KERNEL_LOAD_ADDRESS as *mut u8;
    let mut idle_time: u32 = 0;
    let mut count: u32 = 0;
    loop {
        unsafe {
            let byte = recv_non_blocking();
            match byte {
                Some(byte) => {
                    idle_time = 0;
                    count += 1;
                    if count % 1024 == 0 {
                        print(".");
                    }
                    *ptr = byte;
                    ptr = ptr.offset(1);
                }
                None => {
                    if count == 0 {
                        continue;
                    }
                    idle_time += 1;
                    if idle_time > 100000 {
                        println("");
                        return;
                    }
                }
            }
        }
    }

}