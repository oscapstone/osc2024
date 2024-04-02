#ifndef _DEF_TYPE
#define _DEF_TYPE

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long   uint64_t;

uint32_t swap_endian(uint32_t);

// https://elixir.free-electrons.com/linux/v4.15-rc9/source/scripts/dtc/dtc.h#L72
#define PADDING_4(var) (((var) + 3) & (~0x03))

#endif

