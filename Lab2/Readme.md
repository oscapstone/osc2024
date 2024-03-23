
### Relocation
Rpi 3b+ will automatically load kernel into 0x80000, so the bootloader should relocate itself to 0x60000 to preserve the memory for the kernel, and since the linker linked it to 0x60000, we should load the bootloader to 0x60000 for normal performance