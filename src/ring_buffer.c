#include "malloc.h"
#include "ring_buffer.h"
#include "uart.h"

void rb_init(rbuffer_t *buf, int size)
{
    buf->size = size;
    buf->start = 0;
    buf->end = 0;
    buf->ele = (char *) malloc(sizeof(char) * size);
    return buf;
}

// Use these check on your own
int rb_empty(rbuffer_t *buf)
{
    return buf->start == buf->end;
}

// Use these check on your own
int rb_full(rbuffer_t *buf)
{
    return (buf->end + 1) % buf->size == buf->start;
}

void rb_write(rbuffer_t *buf, char c)
{
    buf->ele[buf->end] = c;
    buf->end = (buf->end+1) % buf->size;
}

char rb_read(rbuffer_t *buf)
{
    char c;
    int i = buf->start;
    buf->start = (i+1) % buf->size;
    return buf->ele[i];
}

void rb_print(rbuffer_t *buf)
{
    uart_puts("ring buffer: ");
    int i = buf->start;
    while (i != buf->end) {
        uart_send(buf->ele[i]);
        uart_send(',');
        i = (i+1) % buf->size;
    }
    uart_send('\n');
}
