#![no_std]
#![feature(start)]

mod panic;
mod stdio;
mod syscall;

use core::arch::asm;
use stdio::{print, print_dec, print_hex, print_u64, println};

fn delay(n: u64) {
    for _ in 0..n {
        unsafe {
            asm!("nop");
        }
    }
}
#[start]
fn main(_: isize, _: *const *const u8) -> isize {
    thread_test();
    syscall::exit(0);
    return 0;
}

fn thread_test() {
    // println("Thread Test");
    for i in 0..10000 {
        let mut pid = syscall::get_pid();
        pid += 1;
        // print_u64("PID", pid);
        // print("  ");
        // print_u64("i", i);
        // println("");

        delay(100000);
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
        print("[Parent] SP=");
        print_hex(sp);
        print(", child PID=");
        print_hex(child_pid);
        println("");
    }
}

// void fork_test(){
//     printf("\nFork Test, pid %d\n", get_pid());
//     int cnt = 1;
//     int ret = 0;
//     if ((ret = fork()) == 0) { // child
//         long long cur_sp;
//         asm volatile("mov %0, sp" : "=r"(cur_sp));
//         printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
//         ++cnt;

//         if ((ret = fork()) != 0){
//             asm volatile("mov %0, sp" : "=r"(cur_sp));
//             printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
//         }
//         else{
//             while (cnt < 5) {
//                 asm volatile("mov %0, sp" : "=r"(cur_sp));
//                 printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
//                 delay(1000000);
//                 ++cnt;
//             }
//         }
//         exit();
//     }
//     else {
//         printf("parent here, pid %d, child %d\n", get_pid(), ret);
//     }
// }
