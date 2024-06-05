#include "relocate.h"

void relocate(char *arg)
{
    unsigned long bootloader_size = (&__end - &__start);
    char *oldbootloader = (char *)&__start;
    char *newbootloader = (char *)&__bootloader_start;

    unsigned long bl_ptr = 0;
    while (bootloader_size--)
    {
        newbootloader[bl_ptr] = oldbootloader[bl_ptr];
        ++bl_ptr;
    }

    void (*run)(char *) = (void (*)(char *))newbootloader;
    run(arg);
}