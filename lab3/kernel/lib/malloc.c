

extern char __heap_start;

char *__heap_top = &__heap_start;

// return current heap pointer
// move heap pointer
void *smalloc(unsigned long size)
{
    //uart_puts("malloc size : ");
    //uart_dec(size);
    //uart_puts("\n");
    char *r = __heap_top;
    size = (size + 0x10 - 1) / 0x10 * 0x10;
    __heap_top += size;
    return r;
}

void *memcpy(void *dest, const void *src, int n) {
    char *cdest = (char *) dest;
    const char *csrc = (const char *) src;

    for (int i = 0; i < n; i++) {
        cdest[i] = csrc[i];
    }

    return dest;
}