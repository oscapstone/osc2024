#include "kernel/gpio.h"
// inline should declare here instead of the header
inline void mmio_write(long reg, unsigned int data){ //MMIO(Memory Mapped IO) : all interactions with hardware on the Raspberry Pi occur using MMIO.
    //vollatile: get the variable from memory directly, instead from register(which may resulted from compiler optimization)
    //see: https://ithelp.ithome.com.tw/articles/10308388
    *(volatile unsigned int*)reg = data;
}

inline unsigned int mmio_read(long reg){
    return *(volatile unsigned int*)reg;
}