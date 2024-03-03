#include "loadimg.h"

void main() {
    loadimg();
    asm volatile(
        "mov x0, x10;"
        "mov x1, x11;"
        "mov x2, x12;"
        "mov x3, x13;"
        "mov x30, 0x60000;"
        "ret;");
}