#![no_std]

const KVA: u64 = 0xffff000000000000;
const MMIO_BASE: u64 = KVA + 0x3f000000;

// SD card command
const GO_IDLE_STATE: u32 = 0;
const SEND_OP_CMD: u32 = 1;
const ALL_SEND_CID: u32 = 2;
const SEND_RELATIVE_ADDR: u32 = 3;
const SELECT_CARD: u32 = 7;
const SEND_IF_COND: u32 = 8;
const VOLTAGE_CHECK_PATTERN: u32 = 0x1aa;
const STOP_TRANSMISSION: u32 = 12;
const SET_BLOCKLEN: u32 = 16;
const READ_SINGLE_BLOCK: u32 = 17;
const WRITE_SINGLE_BLOCK: u32 = 24;
const SD_APP_OP_COND: u32 = 41;
const SDCARD_3_3V: u32 = 1 << 21;
const SDCARD_ISHCS: u32 = 1 << 30;
const SDCARD_READY: u32 = 1 << 31;
const APP_CMD: u32 = 55;

// gpio
const GPIO_BASE: u64 = MMIO_BASE + 0x200000;
const GPIO_GPFSEL4: u64 = GPIO_BASE + 0x10;
const GPIO_GPFSEL5: u64 = GPIO_BASE + 0x14;
const GPIO_GPPUD: u64 = GPIO_BASE + 0x94;
const GPIO_GPPUDCLK1: u64 = GPIO_BASE + 0x9c;

// sdhost
const SDHOST_BASE: u64 = MMIO_BASE + 0x202000;
const SDHOST_CMD: u64 = SDHOST_BASE;
const SDHOST_READ: u32 = 0x40;
const SDHOST_WRITE: u32 = 0x80;
const SDHOST_LONG_RESPONSE: u32 = 0x200;
const SDHOST_NO_REPONSE: u32 = 0x400;
const SDHOST_BUSY: u32 = 0x800;
const SDHOST_NEW_CMD: u32 = 0x8000;
const SDHOST_ARG: u64 = SDHOST_BASE + 0x4;
const SDHOST_TOUT: u64 = SDHOST_BASE + 0x8;
const SDHOST_TOUT_DEFAULT: u32 = 0xf00000;
const SDHOST_CDIV: u64 = SDHOST_BASE + 0xc;
const SDHOST_CDIV_MAXDIV: u32 = 0x7ff;
const SDHOST_CDIV_DEFAULT: u32 = 0x148;
const SDHOST_RESP0: u64 = SDHOST_BASE + 0x10;
const SDHOST_RESP1: u64 = SDHOST_BASE + 0x14;
const SDHOST_RESP2: u64 = SDHOST_BASE + 0x18;
const SDHOST_RESP3: u64 = SDHOST_BASE + 0x1c;
const SDHOST_HSTS: u64 = SDHOST_BASE + 0x20;
const SDHOST_HSTS_MASK: u32 = 0x7f8;
const SDHOST_HSTS_ERR_MASK: u32 = 0xf8;
const SDHOST_HSTS_DATA: u32 = 1 << 0;
const SDHOST_PWR: u64 = SDHOST_BASE + 0x30;
const SDHOST_DBG: u64 = SDHOST_BASE + 0x34;
const SDHOST_DBG_FSM_DATA: u32 = 1;
const SDHOST_DBG_FSM_MASK: u32 = 0xf;
const SDHOST_DBG_MASK: u32 = (0x1f << 14) | (0x1f << 9);
const SDHOST_DBG_FIFO: u32 = (0x4 << 14) | (0x4 << 9);
const SDHOST_CFG: u64 = SDHOST_BASE + 0x38;
const SDHOST_CFG_DATA_EN: u32 = 1 << 4;
const SDHOST_CFG_SLOW: u32 = 1 << 3;
const SDHOST_CFG_INTBUS: u32 = 1 << 1;
const SDHOST_SIZE: u64 = SDHOST_BASE + 0x3c;
const SDHOST_DATA: u64 = SDHOST_BASE + 0x40;
const SDHOST_CNT: u64 = SDHOST_BASE + 0x50;

// helper functions
#[inline(always)]
fn set(io_addr: u64, val: u32) {
    unsafe {
        core::ptr::write_volatile(io_addr as *mut u32, val);
    }
}

#[inline(always)]
fn get(io_addr: u64) -> u32 {
    unsafe {
        core::ptr::read_volatile(io_addr as *mut u32)
    }
}

#[inline(always)]
fn delay(tick: u64) {
    for _ in 0..tick {
        unsafe { asm!("nop"); }
    }
}

static mut IS_HCS: bool = false;

fn pin_setup() {
    set(GPIO_GPFSEL4, 0x24000000);
    set(GPIO_GPFSEL5, 0x924);
    set(GPIO_GPPUD, 0);
    delay(15000);
    set(GPIO_GPPUDCLK1, 0xffffffff);
    delay(15000);
    set(GPIO_GPPUDCLK1, 0);
}

fn sdhost_setup() {
    let mut tmp;
    set(SDHOST_PWR, 0);
    set(SDHOST_CMD, 0);
    set(SDHOST_ARG, 0);
    set(SDHOST_TOUT, SDHOST_TOUT_DEFAULT);
    set(SDHOST_CDIV, 0);
    set(SDHOST_HSTS, SDHOST_HSTS_MASK);
    set(SDHOST_CFG, 0);
    set(SDHOST_CNT, 0);
    set(SDHOST_SIZE, 0);
    tmp = get(SDHOST_DBG);
    tmp &= !SDHOST_DBG_MASK;
    tmp |= SDHOST_DBG_FIFO;
    set(SDHOST_DBG, tmp);
    delay(250000);
    set(SDHOST_PWR, 1);
    delay(250000);
    set(SDHOST_CFG, SDHOST_CFG_SLOW | SDHOST_CFG_INTBUS | SDHOST_CFG_DATA_EN);
    set(SDHOST_CDIV, SDHOST_CDIV_DEFAULT);
}

fn wait_sd() -> i32 {
    let mut cnt = 1000000;
    let mut cmd;
    while cnt != 0 {
        cmd = get(SDHOST_CMD);
        if cmd & SDHOST_NEW_CMD == 0 {
            return 0;
        }
        cnt -= 1;
    }
    -1
}

fn sd_cmd(cmd: u32, arg: u32) -> i32 {
    set(SDHOST_ARG, arg);
    set(SDHOST_CMD, cmd | SDHOST_NEW_CMD);
    wait_sd()
}

fn sdcard_setup() -> i32 {
    let mut tmp;
    sd_cmd(GO_IDLE_STATE | SDHOST_NO_REPONSE, 0);
    sd_cmd(SEND_IF_COND, VOLTAGE_CHECK_PATTERN);
    tmp = get(SDHOST_RESP0);
    if tmp != VOLTAGE_CHECK_PATTERN {
        return -1;
    }
    loop {
        if sd_cmd(APP_CMD, 0) == -1 {
            continue;
        }
        sd_cmd(SD_APP_OP_COND, SDCARD_3_3V | SDCARD_ISHCS);
        tmp = get(SDHOST_RESP0);
        if tmp & SDCARD_READY != 0 {
            break;
        }
        delay(1000000);
    }
    unsafe {
        IS_HCS = tmp & SDCARD_ISHCS != 0;
    }
    sd_cmd(ALL_SEND_CID | SDHOST_LONG_RESPONSE, 0);
    sd_cmd(SEND_RELATIVE_ADDR, 0);
    tmp = get(SDHOST_RESP0);
    sd_cmd(SELECT_CARD, tmp);
    sd_cmd(SET_BLOCKLEN, 512);
    0
}

fn wait_fifo() -> i32 {
    let mut cnt = 1000000;
    let mut hsts;
    while cnt != 0 {
        hsts = get(SDHOST_HSTS);
        if hsts & SDHOST_HSTS_DATA != 0 {
            return 0;
        }
        cnt -= 1;
    }
    -1
}

fn set_block(size: u32, cnt: u32) {
    set(SDHOST_SIZE, size);
    set(SDHOST_CNT, cnt);
}

fn wait_finish() {
    let mut dbg;
    loop {
        dbg = get(SDHOST_DBG);
        if dbg & SDHOST_DBG_FSM_MASK == SDHOST_HSTS_DATA {
            break;
        }
    }
}

pub fn readblock(block_idx: u32, buf: &mut [u32; 128]) {
    let mut succ = false;
    let block_idx = if unsafe { IS_HCS } { block_idx } else { block_idx << 9 };
    while !succ {
        set_block(512, 1);
        sd_cmd(READ_SINGLE_BLOCK | SDHOST_READ, block_idx);
        for i in 0..128 {
            wait_fifo();
            buf[i] = get(SDHOST_DATA);
        }
        let hsts = get(SDHOST_HSTS);
        if hsts & SDHOST_HSTS_ERR_MASK != 0 {
            set(SDHOST_HSTS, SDHOST_HSTS_ERR_MASK);
            sd_cmd(STOP_TRANSMISSION | SDHOST_BUSY, 0);
        } else {
            succ = true;
        }
    }
    wait_finish();
}

pub fn writeblock(block_idx: u32, buf: &[u32; 128]) {
    let mut succ = false;
    let block_idx = if unsafe { IS_HCS } { block_idx } else { block_idx << 9 };
    while !succ {
        set_block(512, 1);
        sd_cmd(WRITE_SINGLE_BLOCK | SDHOST_WRITE, block_idx);
        for i in 0..128 {
            wait_fifo();
            set(SDHOST_DATA, buf[i]);
        }
        let hsts = get(SDHOST_HSTS);
        if hsts & SDHOST_HSTS_ERR_MASK != 0 {
            set(SDHOST_HSTS, SDHOST_HSTS_ERR_MASK);
            sd_cmd(STOP_TRANSMISSION | SDHOST_BUSY, 0);
        } else {
            succ = true;
        }
    }
    wait_finish();
}

pub fn sd_init() {
    pin_setup();
    sdhost_setup();
    sdcard_setup();
}
