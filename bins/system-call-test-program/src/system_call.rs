extern "C" {
    pub fn system_call(
        number: u64,
        arg0: usize,
        arg1: usize,
        arg2: usize,
        arg3: usize,
        arg4: usize,
        arg5: usize,
    ) -> usize;
}

pub fn get_pid() -> i32 {
    unsafe { system_call(0, 0, 0, 0, 0, 0, 0) as i32 }
}

pub fn uart_read(buf: *mut u8, len: usize) -> i32 {
    unsafe { system_call(1, buf as usize, len, 0, 0, 0, 0) as i32 }
}

pub fn uart_write(buf: *const u8, len: usize) -> usize {
    unsafe { system_call(2, buf as usize, len, 0, 0, 0, 0) }
}

pub fn exec(name: *const u8, argv: *const *const u8) -> usize {
    unsafe { system_call(3, name as usize, argv as usize, 0, 0, 0, 0) }
}

pub fn fork() -> i32 {
    unsafe { system_call(4, 0, 0, 0, 0, 0, 0) as i32 }
}

pub fn exit(status: i32) {
    unsafe { system_call(5, status as usize, 0, 0, 0, 0, 0) };
}

pub fn mbox_call(channel: u8, mbox: *mut u32) -> i32 {
    unsafe { system_call(6, channel as usize, mbox as usize, 0, 0, 0, 0) as i32 }
}

pub fn kill(pid: i32) {
    unsafe { system_call(7, pid as usize, 0, 0, 0, 0, 0) };
}

pub fn signal(signal: i32, handler: fn()) {
    unsafe { system_call(8, signal as usize, handler as usize, 0, 0, 0, 0) };
}

pub fn kill_with_signal(pid: i32, signal: i32) {
    unsafe { system_call(9, pid as usize, signal as usize, 0, 0, 0, 0) };
}

pub fn mount(
    src: *const i8,
    target: *const i8,
    filesystem: *const i8,
    flags: u32,
    data: *const (),
) -> i32 {
    unsafe {
        system_call(
            16,
            src as usize,
            target as usize,
            filesystem as usize,
            flags as usize,
            data as usize,
            0,
        ) as i32
    }
}
