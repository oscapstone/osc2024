#pragma once

#include "base.h"
#include "bcm2837base.h"

// on bcm2837 arm peripherals pg. 90
struct GpioPinData {
    REG32 reserved;
    REG32 data[2];
};

struct GpioRegs
{
    REG32 func_select[6];
    struct GpioPinData output_set;
    struct GpioPinData output_clear;
    struct GpioPinData level;
    struct GpioPinData ev_detect_status;    // event detect status
    struct GpioPinData re_detect_enable;    // rising edge detect enable
    struct GpioPinData fe_detect_enable;    // falling edge detect enable
    struct GpioPinData hi_detect_enable;    // high detect enable
    struct GpioPinData lo_detect_enable;    // low detect enable
    struct GpioPinData asyc_re_detect;      // async rising edge detect
    struct GpioPinData asyc_fe_detect;      // async falling edge detect
    REG32 reserved;
    REG32 pull_ud_enable;                   // pull up down enable
    REG32 pull_ud_enable_clock[2];          // pull up down enable clock 0
};

// offset is 0x20 0000
#define REGS_GPIO ((struct GpioRegs *)(PBASE + 0x200000))

// pg. 92 function select
typedef enum _GpioFunc {
    GFInput = 0,
    GFOutput = 1,
    GFAlt0 = 4,
    GFAlt1 = 5,
    GFAlt2 = 6,
    GFAlt3 = 7,
    GFAlt4 = 3,
    GFAlt5 = 2
} GpioFunc;

void gpio_set_func(U8 pinNumber, GpioFunc func);
void gpio_enable(U8 pinNumber);