#include "fifo_buffer.h"


void fifo_init(struct FIFO_BUFFER* fifo, U32 size, void* buffer) {
    fifo->size = size;
    fifo->buffer = buffer;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
}

int fifo_put(struct FIFO_BUFFER* fifo, unsigned char data) {
    if (fifo->free == 0) {
        fifo->flags |= FIFO_OVERRUN;
        return -1;
    }
    fifo->buffer[fifo->p++] = data;

    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }
    fifo->free--;

    return 0;
}
U8 fifo_get(struct FIFO_BUFFER* fifo) {
    if (fifo->free == fifo->size) {
        fifo->flags |= FIFO_UNDERFLOW;
        return -1;
    }
    U8 data = fifo->buffer[fifo->q++];

    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

U32 fifo_status(struct FIFO_BUFFER* fifo) {
    return fifo->size - fifo->free;
}