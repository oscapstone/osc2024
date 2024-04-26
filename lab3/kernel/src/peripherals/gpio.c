
#include "peripherals/gpio.h"
#include "utils/utils.h"


void gpio_set_func(U8 pinNumber, GpioFunc func) {
    U8 bitStart = (pinNumber * 3) % 30;
    U8 reg = pinNumber / 10;
    //U8 reg = 1;

    register U32 selector = REGS_GPIO->func_select[reg];
    selector &= ~(7 << bitStart);
    selector |= (func << bitStart);

    REGS_GPIO->func_select[reg] = selector;
}

void gpio_enable(U8 pinNumber) {
    REGS_GPIO->pull_ud_enable = 0;
    utils_delay(150);
    REGS_GPIO->pull_ud_enable_clock[pinNumber / 32] = 1 << (pinNumber % 32);
    utils_delay(150);
    REGS_GPIO->pull_ud_enable = 0;
    REGS_GPIO->pull_ud_enable_clock[pinNumber / 32] = 0;
}