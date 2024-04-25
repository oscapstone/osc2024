#include "type.h"

uint32_t swap_endian(uint32_t n)
{
    uint32_t b0, b1, b2, b3;
    b3 = (n & 0x000000ff) << 24;
    b2 = (n & 0x0000ff00) << 8;
    b1 = (n & 0x00ff0000) >> 8;
    b0 = (n & 0xff000000) >> 24;

    return b3 | b2 | b1 | b0;
}
