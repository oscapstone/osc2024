#![no_std]
#![feature(start)]

mod panic;
mod stdio;
mod syscall;

use core::arch::asm;
use stdio::{print, print_dec, print_hex, print_u64, println};

fn delay(n: u64) {
    unsafe {
        let frq: u64;
        asm!("mrs {}, cntfrq_el0", out(reg) frq);
        let pct: u64;
        asm!("mrs {}, cntpct_el0", out(reg) pct);
        print("frq=");
        print_hex(frq);
        print(", pct=");
        print_hex(pct);
        println("");

        let mut cur: u64 = 0;
        while pct + n * frq / 1000 > cur {
            asm!("mrs {}, cntpct_el0", out(reg) cur);
        }
    }
}
#[start]
fn main(_: isize, _: *const *const u8) -> isize {
    basic_test();
    syscall::exit(0);
    return 0;
}

#[allow(dead_code)]
fn basic_test() {
    // println("Hello, world!");
    let pid = syscall::get_pid();
    print("PID=");
    print_hex(pid);
    println("");
}

#[allow(dead_code)]
fn mailbox_test() {
    println("Printing mailbox info...");
    let revision = get_board_revision();
    print("Board revision: ");
    print_hex(revision);
    println("");
    let (lb, ub) = get_arm_memory();
    print("ARM memory: ");
    print_hex(lb);
    print(" - ");
    print_hex(ub);
    println("");
    delay(100);
}

pub fn get_board_revision() -> u64 {
    let mut mailbox = [0; 7];
    mailbox[0] = 7 * 4; // Buffer size in bytes
    mailbox[1] = 0; // Request/response code
    mailbox[2] = 0x0001_0002; // Tag: Get board revision
    mailbox[3] = 4; // Buffer size in bytes
    mailbox[4] = 0; // Tag request code
    mailbox[5] = 0; // Value buffer
    mailbox[6] = 0x0000_0000; // End tag
    let mut mailbox = MailBox::new(&mailbox);
    assert!(mailbox.call(8), "Failed to get board revision");
    mailbox.get(5)
}

pub fn get_arm_memory() -> (u64, u64) {
    let mut mailbox = [0; 8];
    mailbox[0] = 8 * 4; // Buffer size in bytes
    mailbox[1] = 0; // Request/response code
    mailbox[2] = 0x0001_0005; // Tag: Get ARM memory
    mailbox[3] = 8; // Buffer size in bytes
    mailbox[4] = 0; // Tag request code
    mailbox[5] = 0; // Value buffer
    mailbox[6] = 0; // Value buffer
    mailbox[7] = 0x0000_0000; // End tag
    let mut mailbox = MailBox::new(&mailbox);
    assert!(mailbox.call(8), "Failed to get ARM memory");
    (mailbox.get(5), mailbox.get(6))
}

#[allow(dead_code)]
fn fork_bump() {
    let pid = syscall::fork();
    if pid == 0 {
        println("I'm the child");
    } else {
        println("I'm the parent");
    }
}

#[allow(dead_code)]
fn thread_test() {
    println("Thread Test");
    for i in 0..10000000 {
        let pid = syscall::get_pid();
        print_u64("PID", pid);
        print("  ");
        print_u64("i", i);
        println("");
        delay(1);
    }
}

#[allow(dead_code)]
fn fork_test() {
    let mut cnt = 0;
    println("[program] Hello, world!");
    let pid = syscall::get_pid();
    print("[program] PID=");
    print_hex(pid);
    println("");
    let child_pid = syscall::fork();
    if child_pid == 0 {
        println("[Child] I'm the child");
        let sp: u64;
        unsafe {
            asm!("mov {}, sp", out(reg) sp);
        }
        print("[Child] SP=");
        print_hex(sp);
        println("");
        let child_pid2 = syscall::fork();
        if child_pid2 != 0 {
            println("[Child] I'm the parent of the second child");
            let sp: u64;
            unsafe {
                asm!("mov {}, sp", out(reg) sp);
            }
            print("[Child] SP=");
            print_hex(sp);
            print(", child PID=");
            print_hex(child_pid2);
            print(", cnt_ptr=0x");
            print_hex(&mut cnt as *mut _ as u64);
            println("");
        } else {
            println("[Child2] I'm the second child");
            while cnt < 10 {
                let sp: u64;
                unsafe {
                    asm!("mov {}, sp", out(reg) sp);
                }
                print("[Child2] SP=");
                print_hex(sp);
                print(", cnt=");
                print_dec(cnt);
                print(", cnt_ptr=0x");
                print_hex(&mut cnt as *mut _ as u64);
                println("");
                cnt += 1;
            }
        }
    } else {
        println("[Parent] I'm the parent");
        let sp: u64;
        unsafe {
            asm!("mov {}, sp", out(reg) sp);
        }
        delay(10000000000);
        print("[Parent] SP=");
        print_hex(sp);
        print(", child PID=");
        print_hex(child_pid);
        println("");
    }
}

#[repr(C, align(16))]
pub struct MailBox {
    buffer: [u32; 36],
    pub len: u32,
}

impl MailBox {
    pub fn new(buf: &[u32]) -> Self {
        let mut buffer = [0; 36];
        let len = buf[0] as usize / 4;
        for i in 0..len {
            buffer[i] = buf[i] as u32;
        }
        MailBox {
            buffer,
            len: len as u32,
        }
    }

    pub fn call(&mut self, channel: u8) -> bool {
        syscall::mbox_call(channel, self.buffer.as_mut_ptr()) == 1
    }

    pub fn get(&self, index: usize) -> u64 {
        self.buffer[index] as u64
    }
}
