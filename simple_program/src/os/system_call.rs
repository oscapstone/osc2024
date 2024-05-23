use core::arch::asm;

pub fn get_pid() -> u32 {
    let pid: u32;
    unsafe {
        asm!(
            "ldr x8, =0x00",
            "svc 0",
            out("x0") pid,
            out("x8") _,
        )
    }
    pid
}

pub fn uart_read(buf: *mut u8, size: usize) -> usize {
    let read_size: usize;
    unsafe {
        asm!(
            "mov x0, {buf}",
            "mov x1, {size}",
            "ldr x8, =0x01",
            "svc 0",
            buf = in(reg) buf,
            size = in(reg) size,
            out("x0") read_size,
            out("x1") _,
            out("x8") _,
            options(nostack),
        )
    }
    read_size
}

pub fn uart_write(buf: *const u8, size: usize) -> usize {
    let write_size: usize;
    unsafe {
        asm!(
            "mov x0, {buf}",
            "mov x1, {size}",
            "ldr x8, =0x02",
            "svc 0",
            buf = in(reg) buf,
            size = in(reg) size,
            out("x0") write_size,
            out("x1") _,
            out("x8") _,
            options(nostack),
        )
    }
    write_size
}

pub fn exec(name: *const u8, args: *const *const u8) -> u32 {
    let pid: u32;
    unsafe {
        asm!(
            "mov x0, {name}",
            "mov x1, {args}",
            "ldr x8, =0x03",
            "svc 0",
            name = in(reg) name,
            args = in(reg) args,
            out("x0") pid,
            out("x1") _,
            out("x8") _,
        )
    }
    pid
}

pub fn fork() -> u32 {
    let pid: u32;
    unsafe {
        asm!(
            "ldr x8, =0x04",
            "svc 0",
            out("x0") pid,
            out("x8") _,
        )
    }
    pid
}

pub fn exit() {
    unsafe {
        asm!(
            "ldr x8, =0x05",
            "svc 0",
            out("x8") _,
        )
    }
}

// Get the hardware’s information by mailbox
pub fn mbox_call(ch: u8, mbox: *mut u32) -> u32 {
    let ret: u32;
    unsafe {
        asm!(
            "mov x0, {ch}",
            "mov x1, {mbox}",
            "ldr x8, =0x06",
            "svc 0",
            ch = in(reg) ch,
            mbox = in(reg) mbox,
            out("x0") ret,
            out("x1") _,
            out("x8") _,
        )
    }
    ret
}

pub fn kill(pid: u32) {
    unsafe {
        asm!(
            "mov x0, {pid}",
            "ldr x8, =0x07",
            "svc 0",
            pid = in(reg) pid,
            out("x0") _,
            out("x8") _,
        )
    }
}