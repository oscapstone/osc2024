#include "mmio.h"

inline void mmio_write(long reg, unsigned int data){
    *(volatile unsigned int*)reg = data;
}

inline unsigned int mmio_read(long reg){
    return *(volatile unsigned int*)reg;
}
