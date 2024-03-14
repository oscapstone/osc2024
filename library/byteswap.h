#ifndef BYTESWAP_H
#define BYTESWAP_H



#define bswap_32(num) ((num >> 24) | (num << 24) | ((num & 0x0000ff00) << 8) | ((num & 0x00ff0000) >> 8))



#endif