#ifndef MMIO_H
#define MMIO_H

void mmio_write(long reg, unsigned int data);

unsigned int mmio_read(long reg);

#endif
