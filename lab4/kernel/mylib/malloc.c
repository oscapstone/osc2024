extern char __startup_allocator_start;
char * startup_allocator_start = (char *) &__startup_allocator_start;

void *simple_malloc(unsigned long size)
{
    char *r = startup_allocator_start;
    //
    size = (size + 0x10 - 1) / 0x10 * 0x10; //對齊0x10
    startup_allocator_start += size;
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
