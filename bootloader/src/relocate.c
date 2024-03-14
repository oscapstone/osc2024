extern unsigned char _begin, _end, __boot_loader;

void relocate()
{
    int kernel_size = (&_end - &_begin);
    char *new_bl = (char*) (&__boot_loader);
    char *bl = (char *)&_begin;

    for (int i = 0; i <= kernel_size; i++)
        *new_bl++ = *bl++;

    void (*start)(void) = (void *) &__boot_loader;
    start();
}