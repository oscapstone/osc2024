extern char* _bootloader_relocated_addr;
extern unsigned long long _bootloader_size;
extern unsigned long long _after_relocate;

__attribute__((section(".text.relocate"))) void relocate()
{
	unsigned long long size = (unsigned long long)&_bootloader_size;
	char* relocated_ptr = (char*)&_bootloader_relocated_addr;
	char *begin = (char *)&_after_relocate;
	for (unsigned long long i = 0; i < size; i++)
	{
		relocated_ptr[i] = begin[i];
	}

	((void (*)())relocated_ptr)();

	// ((void (*)())addr)();
}