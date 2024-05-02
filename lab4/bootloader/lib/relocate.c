// from linker script
extern char __end;
extern char __start;

void relocate(char *arg)
{
    unsigned long bootloader_size = (&__end - &__start);
    char *oldbootloader = (char *)&__start; //0x80000
    char *newbootloader = (char *)0x60000;

    unsigned long bl_ptr = 0;
    // copying
    while (bootloader_size--)
    {
        newbootloader[bl_ptr] = oldbootloader[bl_ptr];
        ++bl_ptr;
    }
    // run kernel in 0x60000
    void (*run)(char *) = (void (*)(char *))newbootloader;
    run(arg);
}
// can be verifying in gdb