use crate::scheduler;
use stdio::println;
pub fn get_pid() -> u64 {
    scheduler::get().current.unwrap() as u64
}

pub fn read(buf: *mut u8, size: usize) -> usize {
    let mut read = 0;
    for i in 0..size {
        unsafe {
            if let Some(c) = driver::uart::recv_async() {
                *buf.add(i) = c;
                read += 1;
            } else {
                break;
            }
        }
    }
    read
}

pub fn write(buf: *const u8, size: usize) -> usize {
    let mut written = 0;
    for i in 0..size {
        unsafe {
            let c = *buf.add(i);
            stdio::send(c);
            // driver::uart::send(c);
        }
        written += 1;
    }
    written
}

pub fn exec(name: *const u8) -> u64 {
    println!("exec: {:?}", name);
    0
}

pub fn fork() -> u64 {
    scheduler::get().fork()
}

pub fn exit(status: u64) {
    println!("exit: {}", status);
    crate::scheduler::get().exit(status);
}

pub fn mbox_call(channel: u8, mbox: *mut u32) -> u64 {
    let mut mailbox = [0; 36];
    let len = unsafe { *mbox } as usize / 4;
    for i in 0..len {
        unsafe {
            mailbox[i] = *mbox.add(i);
        }
    }
    let mut mailbox = driver::mailbox::MailBox::new(&mailbox);
    let ret = mailbox.call(channel);

    for i in 0..mailbox.get(0) as usize / 4 {
        unsafe {
            *mbox.add(i) = mailbox.get(i as usize);
        }
    }
    ret as u64
}
