#pragma once

#include "base.h"

#define FIFO_OVERRUN    0x0001
#define FIFO_UNDERFLOW  0x0010

struct FIFO_BUFFER {
    char* buffer;
    U32 p, q, size, free, flags;
};

// assign buffer to the fifo buffer struct
void fifo_init(struct FIFO_BUFFER* fifo, U32 size, char* buffer);

int fifo_put(struct FIFO_BUFFER* fifo, unsigned char data);
U8 fifo_get(struct FIFO_BUFFER* fifo);

// how many data store in buffer
U32 fifo_status(struct FIFO_BUFFER* fifo);