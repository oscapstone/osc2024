# How a Kernel is Loaded on Rpi3

##
1. After powering on, the GPU activates before the ARM CPU and SDRAM.
2. The GPU executes the first stage bootloader from ROM on the SoC to check the FAT16/32 file system on the SD card.
3. The GPU loads the second stage bootloader bootcode.bin from SD card to L2 cache.
4. bootcode.bin activates SDRAM and loads the third stage bootloader loader.bin to RAM.
5. loader.bin loads the GPU firmware start.elf.
6. start.elf reads configuration files config.txt and cmdline.txt, and loads the crucial Linux kernel kernel.img.
7. Once kernel.img is loaded, start.elf activates the CPU, and the kernel starts working.