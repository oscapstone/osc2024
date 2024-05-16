use core::arch::asm;

use crate::{
    println,
    system_call::{exit, fork, get_pid},
};

fn delay(n: usize) {
    for _ in 0..n {
        unsafe {
            asm!("nop");
        }
    }
}

pub fn fork_test() {
    println!("\nFork Test, pid {}", get_pid());
    let mut cnt = 1;
    let ret = fork();
    if ret == 0 {
        let mut cur_sp: usize = 0;
        unsafe {
            asm!("mov {}, sp", out(reg) cur_sp);
        }
        println!(
            "first child pid: {}, cnt: {}, ptr: {:p}, sp : {:#x}",
            get_pid(),
            cnt,
            &cnt,
            cur_sp
        );
        cnt += 1;

        if fork() != 0 {
            unsafe {
                asm!("mov {}, sp", out(reg) cur_sp);
            }
            println!(
                "first child pid: {}, cnt: {}, ptr: {:p}, sp : {:#x}",
                get_pid(),
                cnt,
                &cnt,
                cur_sp
            );
        } else {
            while cnt < 5 {
                unsafe {
                    asm!("mov {}, sp", out(reg) cur_sp);
                }
                println!(
                    "second child pid: {}, cnt: {}, ptr: {:p}, sp : {:#x}",
                    get_pid(),
                    cnt,
                    &cnt,
                    cur_sp
                );
                // delay(1000000);
                delay(10000000);
                cnt += 1;
            }
        }
        exit(0);
    } else {
        println!("parent here, pid {}, child {}", get_pid(), ret)
    }
}
